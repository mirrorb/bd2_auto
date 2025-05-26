use std::sync::{Arc, Mutex};
use tauri_plugin_shell::{process::CommandChild, ShellExt};
use tauri_plugin_shell::process::CommandEvent;
use tauri::{Emitter, Manager};
use tokio::time::{sleep, Duration};

#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

#[derive(Clone)]
struct ProcessState(Arc<Mutex<Option<CommandChild>>>);

impl Default for ProcessState {
    fn default() -> Self {
        ProcessState(Arc::new(Mutex::new(None)))
    }
}

#[tauri::command]
async fn core_start(app: tauri::AppHandle) -> Result<(), String> {
    // 在启动监控任务之前，确保主窗口存在。
    let main_window = app.get_webview_window("main")
        .ok_or_else(|| "Main window not found".to_string())?;

    // 克隆需要在异步任务中使用的句柄
    let app_clone = app.clone();
    let main_window_clone = main_window.clone();

    tauri::async_runtime::spawn(async move {
        let state_handle: tauri::State<'_, ProcessState> = app_clone.state::<ProcessState>();
        let process_state_arc = state_handle.inner().0.clone();

        loop {
            let sidecar_command = match app_clone.shell().sidecar("bd2_auto_core") {
                Ok(cmd) => cmd,
                Err(e) => {
                    let error_msg = format!("Failed to get sidecar command: {}", e);
                    eprintln!("{}", error_msg);
                    // 向前端发送错误。
                    if let Err(emit_err) = main_window_clone.emit("core-error", &error_msg) {
                        eprintln!("Failed to emit core-error: {}", emit_err);
                    }
                    // 等待几秒钟再重试，以避免错误信息刷屏。
                    sleep(Duration::from_secs(5)).await;
                    continue;
                }
            };

            let (mut rx, child) = match sidecar_command.spawn() {
                Ok((rx_process, child_process)) => (rx_process, child_process),
                Err(e) => {
                    let error_msg = format!("Failed to spawn sidecar: {}", e);
                    eprintln!("{}", error_msg);
                    // 向前端发送错误。
                    if let Err(emit_err) = main_window_clone.emit("core-error", &error_msg) {
                        eprintln!("Failed to emit core-error: {}", emit_err);
                    }
                    // 等待再重试。
                    sleep(Duration::from_secs(5)).await;
                    continue;
                }
            };

            // 将新生成的子进程存储在共享状态中。
            // 这允许像 `core_input` 这样的其他命令与其交互。
            *process_state_arc.lock().unwrap() = Some(child);

            // 通知前端 sidecar 进程已启动或已重新启动。
            if let Err(e) = main_window_clone.emit("core-message", "Sidecar process started/restarted.") {
                eprintln!("Failed to emit core-message (start/restart): {}", e);
            }

            // 当前 sidecar 实例的事件循环。
            // 此循环处理 stdout、stderr、错误和终止信号。
            while let Some(event) = rx.recv().await {
                match event {
                    CommandEvent::Stdout(line) => {
                        if let Err(e) = main_window_clone.emit("core-output", String::from_utf8_lossy(&line).to_string()) {
                            eprintln!("Failed to emit core-output: {}", e);
                        }
                    }
                    CommandEvent::Stderr(line) => {
                        let line_str = String::from_utf8_lossy(&line).to_string();
                        eprintln!("Sidecar stderr: {}", line_str);
                        if let Err(e) = main_window_clone.emit("core-error", format!("stderr: {}", line_str)) {
                            eprintln!("Failed to emit core-error (stderr): {}", e);
                        }
                    }
                    CommandEvent::Error(message) => {
                        eprintln!("Sidecar command error: {}", message);
                        if let Err(e) = main_window_clone.emit("core-error", format!("error: {}", message)) {
                            eprintln!("Failed to emit core-error (command error): {}", e);
                        }
                        // 此事件通常在崩溃的 Terminated 事件之前。
                    }
                    CommandEvent::Terminated(payload) => {
                        let termination_msg = format!("Sidecar terminated with payload: {:?}", payload);
                        println!("{}", termination_msg);
                        if let Err(e) = main_window_clone.emit("core-terminated", &termination_msg) {
                            eprintln!("Failed to emit core-terminated: {}", e);
                        }
                        // sidecar 已终止。跳出事件循环
                        // 以允许外部循环尝试重新启动。
                        break;
                    }
                    _ => { /* 其他事件在这里处理。 */ }
                }
            }

            // 如果 sidecar 终止或其事件流结束，则会到达此点。
            println!("Sidecar event stream ended for the current instance.");

            // 由于子进程已终止，从共享状态中清除它。
            *process_state_arc.lock().unwrap() = None;

            // 宣布重启尝试。
            let restart_msg = "Sidecar process ended. Attempting to restart in 5 seconds...";
            eprintln!("{}", restart_msg);
            if let Err(e) = main_window_clone.emit("core-message", restart_msg) {
                 eprintln!("Failed to emit core-message (restarting): {}", e);
            }

            // 在尝试重新启动之前等待几秒钟。
            // 这可以防止在 sidecar 立即崩溃时出现快速重启循环。
            sleep(Duration::from_secs(5)).await;
            // 然后循环将继续下一次迭代，尝试重新启动 sidecar。
        }
    });

    Ok(())
}

#[tauri::command]
async fn core_input(input: String, state: tauri::State<'_, ProcessState>) -> Result<(), String> {
    if let Some(child) = &mut *state.inner().0.lock().unwrap() {
        // 写入到 sidecar 进程的 stdin
        child.write(input.as_bytes())
            .map_err(|e| format!("Failed to write to sidecar stdin: {}", e))?;
        // 如果您的 C++ 程序期望每条输入后都有换行符
        child.write(b"\n")
             .map_err(|e| format!("Failed to write newline to sidecar stdin: {}", e))?;
    } else {
        return Err("Sidecar process not started or already terminated.".to_string());
    }
    Ok(())
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .plugin(tauri_plugin_shell::init())
        .manage(ProcessState::default())
        .invoke_handler(tauri::generate_handler![greet, core_start, core_input])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
