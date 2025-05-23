#include "tasks/hello_task.h"
#include <chrono>

HelloTask::HelloTask(std::string name) : task_name_(std::move(name)) {
    current_task_status_json_["task_name"] = task_name_; // 初始化状态JSON
    current_task_status_json_["status_detail"] = "未开始";
}

HelloTask::~HelloTask() {
    stop(); // 请求停止任务
    if (task_thread_.joinable()) {
        task_thread_.join(); // 等待任务线程结束
    }
}

std::string HelloTask::getTaskName() const {
    return task_name_;
}

bool HelloTask::isRunning() const {
    return is_running_.load(); // 原子操作读取
}

json HelloTask::getStatus() const {
    json status;
    status["task_name"] = task_name_;
    status["is_running"] = is_running_.load();
    status["params"] = params_; // 可以选择性地返回任务启动参数
    status["details"] = current_task_status_json_; // 返回内部维护的状态
    return status;
}

void HelloTask::start(const json& params, std::function<void(const json&)> progress_callback) {
    if (is_running_.load()) {
        if (progress_callback) { // 检查回调是否有效
            json error_msg;
            error_msg["type"] = "log";
            error_msg["level"] = "warn";
            error_msg["message"] = "任务 '" + task_name_ + "' 已在运行中，无法重复启动。";
            progress_callback(error_msg);
        }
        return;
    }
    params_ = params; // 保存参数
    progress_callback_ = progress_callback; // 保存回调
    stop_requested_ = false; // 重置停止请求标志
    is_running_ = true; // 设置运行状态
    current_task_status_json_["status_detail"] = "正在初始化...";

    // 创建并启动任务线程
    task_thread_ = std::thread(&HelloTask::run, this);
}

void HelloTask::stop() {
    stop_requested_ = true; // 设置停止请求标志
    // 实际的停止发生在run()方法内部检查此标志
    current_task_status_json_["status_detail"] = "正在请求停止...";
}

// 任务线程的执行函数
void HelloTask::run() {
    current_task_status_json_["status_detail"] = "已启动";
    sendProgressUpdate(0, "任务启动");

    for (int i = 0; i <= 100; i += 20) {
        if (stop_requested_.load()) { // 检查是否请求停止
            is_running_ = false; // 更新运行状态
            current_task_status_json_["status_detail"] = "已中断";
            sendProgressUpdate(i, "任务被用户中断");
            return; // 退出线程
        }

        // 模拟实际工作 (例如调用 ADBController, ImageProcessor)
        // 在真实应用中，这里会是实际的游戏自动化操作
        current_task_status_json_["current_step_value"] = (i / 20 + 1); // 更新内部状态

        sendProgressUpdate(i, "执行步骤 " + std::to_string(i / 20 + 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 模拟耗时操作
    }

    is_running_ = false; // 任务正常完成，更新运行状态
    current_task_status_json_["status_detail"] = "已完成";
    sendProgressUpdate(100, "任务完成");

    // 任务完成后可以发送一个特定的完成事件
    if (progress_callback_) {
        json completion_event;
        completion_event["type"] = "event";
        completion_event["event_name"] = "task_completed";
        completion_event["task_name"] = task_name_;
        completion_event["data"]["message"] = "任务 '" + task_name_ + "' 成功执行完毕。";
        progress_callback_(completion_event);
    }
}

// 辅助函数，用于发送进度更新
void HelloTask::sendProgressUpdate(int percentage, const std::string& step_message) {
    if (progress_callback_) {
        json j_progress;
        j_progress["type"] = "progress";
        j_progress["task_name"] = task_name_;
        j_progress["current_step"] = step_message;
        j_progress["percentage"] = percentage;
        progress_callback_(j_progress);
    }
}