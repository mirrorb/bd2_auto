#include "basic/task_manager.h"
#include "tasks/hello_task.h"

TaskManager::TaskManager(std::function<void(const json&)> sender)
    : logger_(std::make_shared<Logger>(std::move(sender))) {}

bool TaskManager::startTask(const std::string& task_name, const json& params) {
    if (current_task_ && current_task_->isRunning()) {
        logger_->error("无法启动新任务 '" + task_name + "'，已有任务 '" + current_task_->getTaskName() + "' 正在运行。");
        return false;
    }

    // 根据 task_name 创建相应的任务对象
    if (task_name == "hello_task") {
        current_task_ = std::make_unique<HelloTask>(task_name);
    }
    // else if (task_name == "another_specific_task") {
    //     current_task_ = std::make_unique<AnotherSpecificTask>(task_name);
    // }
    else {
        logger_->error("未知的任务类型: " + task_name);
        return false;
    }

    current_task_->start(params, logger_);
    return true;
}

bool TaskManager::stopCurrentTask() {
    if (current_task_ && current_task_->isRunning()) {
        current_task_->stop();
        logger_->info("已向任务 '" + current_task_->getTaskName() + "' 发送停止请求。");
        return true;
    }
    logger_->info("没有正在运行的任务可以停止。");
    return false;
}

json TaskManager::getStatus() const {
    json status_report;
    if (current_task_) {
        status_report["active_task"] = current_task_->getStatus();
        status_report["message"] = "获取到活动任务 '" + current_task_->getTaskName() + "' 的状态。";
    } else {
        status_report["active_task"] = nullptr; // 表示没有活动任务
        status_report["message"] = "当前没有活动任务。";
    }
    return status_report;
}
