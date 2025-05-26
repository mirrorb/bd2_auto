#include "basic/task_manager.h"
#include "tasks/hello_task.h"
// 如果有其他任务类型，也需要在这里包含

TaskManager::TaskManager(std::function<void(const json&)> progress_callback)
    : global_progress_callback_(std::move(progress_callback)) {}

bool TaskManager::startTask(const std::string& task_name, const json& params) {
    if (current_task_ && current_task_->isRunning()) {
        if (global_progress_callback_) {
            json error_msg;
            error_msg["type"] = "log";
            error_msg["level"] = "error";
            error_msg["message"] = "无法启动新任务 '" + task_name + "'，已有任务 '" + current_task_->getTaskName() + "' 正在运行。";
            global_progress_callback_(error_msg);
        }
        return false;
    }

    // 任务工厂 (简单版本)
    // 根据 task_name 创建相应的任务对象
    if (task_name == "hello_task") {
        current_task_ = std::make_unique<HelloTask>(task_name);
    }
    // else if (task_name == "another_specific_task") {
    //     current_task_ = std::make_unique<AnotherSpecificTask>(task_name);
    // }
    else {
        if (global_progress_callback_) {
            json error_msg;
            error_msg["type"] = "log";
            error_msg["level"] = "error";
            error_msg["message"] = "未知的任务类型: " + task_name;
            global_progress_callback_(error_msg);
        }
        return false; // 未知任务类型，启动失败
    }

    // 将全局回调传递给任务，以便任务内部可以发送消息
    current_task_->start(params, global_progress_callback_);
    return true;
}

bool TaskManager::stopCurrentTask() {
    if (current_task_ && current_task_->isRunning()) {
        current_task_->stop();
        // 发出stop请求
        if (global_progress_callback_) {
            json log_msg;
            log_msg["type"] = "log";
            log_msg["level"] = "info";
            log_msg["message"] = "已向任务 '" + current_task_->getTaskName() + "' 发送停止请求。";
            global_progress_callback_(log_msg);
        }
        return true;
    }
    if (global_progress_callback_) {
        json log_msg;
        log_msg["type"] = "log";
        log_msg["level"] = "info";
        log_msg["message"] = "没有正在运行的任务可以停止。";
        global_progress_callback_(log_msg);
    }
    return false;
}

json TaskManager::getStatus() const {
    json status_report;
    // status_report["type"] = "manager_status"; // 可以给管理器状态加一个顶层类型
    if (current_task_) {
        status_report["active_task"] = current_task_->getStatus();
        status_report["message"] = "获取到活动任务 '" + current_task_->getTaskName() + "' 的状态。";
    } else {
        status_report["active_task"] = nullptr; // 表示没有活动任务
        status_report["message"] = "当前没有活动任务。";
    }
    return status_report;
}