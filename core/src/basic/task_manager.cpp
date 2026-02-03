#include "basic/task_manager.h"
#include "tasks/hello_task.h"
#include "tasks/fishing_task.h"

TaskManager::TaskManager(std::function<void(const json&)> sender)
    : logger_(std::make_shared<Logger>(std::move(sender))) {}

bool TaskManager::startTask(const std::string& task_name, const json& params) {
    if (current_task_ && current_task_->isRunning()) {
        logger_->error("Cannot start task '" + task_name + "': task '" + current_task_->getTaskName() + "' is running.");
        return false;
    }

    if (task_name == "hello_task") {
        current_task_ = std::make_unique<HelloTask>(task_name);
    } else if (task_name == "fishing_task") {
        current_task_ = std::make_unique<FishingTask>(task_name);
    } else {
        logger_->error("Unknown task type: " + task_name);
        return false;
    }

    current_task_->start(params, logger_);
    return true;
}

bool TaskManager::stopCurrentTask() {
    if (current_task_ && current_task_->isRunning()) {
        current_task_->stop();
        logger_->info("Stop requested for task '" + current_task_->getTaskName() + "'.");
        return true;
    }
    logger_->info("No running task to stop.");
    return false;
}

json TaskManager::getStatus() const {
    json status_report;
    if (current_task_) {
        status_report["active_task"] = current_task_->getStatus();
        status_report["message"] = "Status for active task '" + current_task_->getTaskName() + "'.";
    } else {
        status_report["active_task"] = nullptr;
        status_report["message"] = "No active task.";
    }
    return status_report;
}

json TaskManager::getTaskList() const {
    json tasks = json::array();
    tasks.push_back({
        {"name", "fishing_task"},
        {"label", "钓鱼任务"},
        {"queue", {
            {"singleton", true},
            {"must_last", true},
            {"is_looping", true}
        }}
    });
    tasks.push_back({
        {"name", "hello_task"},
        {"label", "测试任务"},
        {"queue", {
            {"singleton", true},
            {"must_last", false},
            {"is_looping", false}
        }}
    });
    json result;
    result["tasks"] = tasks;
    return result;
}
