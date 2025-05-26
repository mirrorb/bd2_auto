// Learn more about Tauri commands at https://tauri.app/develop/calling-rust/
use std::sync::Mutex;
// 导入 Command 和 CommandChild 用于 sidecar
use tauri_plugin_shell::{process::CommandChild, ShellExt}; // ShellExt for app.shell()
use tauri_plugin_shell::process::CommandEvent;
use tauri::{Emitter, Manager}; // Manager 用于 app_handle

#[tauri::command]
fn greet(name: &str) -> String {
    format!("Hello, {}! You've been greeted from Rust!", name)
}

#[derive(Default)]
struct ProcessState(Mutex<Option<CommandChild>>); // ProcessState 现在持有 CommandChild

#[tauri::command]
async fn core_start(app: tauri::AppHandle, state: tauri::State<'_, ProcessState>) -> Result<(), String> {
    // app.shell() 需要 tauri_plugin_shell 已经被初始化
    let sidecar_command = app.shell()
        .sidecar("bd2_auto_core")
        .map_err(|e| format!("Failed to get sidecar command: {}", e))?; // 使用 map_err 转换错误类型

    let (mut rx, child) = sidecar_command
      .spawn()
      .map_err(|e| format!("Failed to spawn sidecar: {}", e))?; // 使用 map_err

    // 确保窗口存在
    let window = app.get_webview_window("main")
        .ok_or_else(|| "Main window not found".to_string())?;

    // 异步任务监听 sidecar 进程的事件 (stdout, stderr, etc.)
    tauri::async_runtime::spawn(async move {
        while let Some(event) = rx.recv().await {
            match event {
                CommandEvent::Stdout(line) => {
                    if let Err(e) = window.emit("core-output", String::from_utf8_lossy(&line).to_string()) {
                        eprintln!("Failed to emit core-output: {}", e);
                    }
                }
                CommandEvent::Stderr(line) => {
                    let line_str = String::from_utf8_lossy(&line).to_string();
                    eprintln!("Sidecar stderr: {}", line_str); // 记录到控制台
                    if let Err(e) = window.emit("core-error", format!("stderr: {}", line_str)) {
                        eprintln!("Failed to emit core-error (stderr): {}", e);
                    }
                }
                CommandEvent::Error(message) => {
                    eprintln!("Sidecar command error: {}", message);
                    if let Err(e) = window.emit("core-error", format!("error: {}", message)) {
                        eprintln!("Failed to emit core-error (command error): {}", e);
                    }
                }
                CommandEvent::Terminated(payload) => {
                    println!("Sidecar terminated with payload: {:?}", payload);
                    if let Err(e) = window.emit("core-terminated", format!("terminated: {:?}", payload)) {
                        eprintln!("Failed to emit core-terminated: {}", e);
                    }
                }
                _ => {
                    // 其他事件，例如 Pid
                }
            }
        }
        println!("Sidecar event stream ended."); // 当进程结束且所有事件处理完毕时
    });

    *state.0.lock().unwrap() = Some(child);
    Ok(())
}

#[tauri::command]
async fn core_input(input: String, state: tauri::State<'_, ProcessState>) -> Result<(), String> {
    if let Some(child) = &mut *state.0.lock().unwrap() {
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
        .plugin(tauri_plugin_shell::init()) // <--- 添加这一行来初始化 shell 插件
        .manage(ProcessState::default())     // 这个是你自定义的状态，与 shell 插件的状态是分开的
        .invoke_handler(tauri::generate_handler![greet, core_start, core_input])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}