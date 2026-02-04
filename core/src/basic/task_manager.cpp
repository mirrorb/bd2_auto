#include "basic/task_manager.h"
#include "tasks/hello_task.h"
#include "tasks/fishing_task.h"
#include "io/window_handler.h"
#include "basic/exceptions.h"

TaskManager::TaskManager(std::function<void(const json&)> sender)
    : logger_(std::make_shared<Logger>(std::move(sender))) {}

bool TaskManager::startTask(const std::string& task_name, const json& params) {
    last_error_.clear();
    if (current_task_ && current_task_->isRunning()) {
        last_error_ = "Cannot start task '" + task_name + "': task '" + current_task_->getTaskName() + "' is running.";
        return false;
    }

    const bool is_hello = (task_name == "hello_task");
    const bool is_fishing = (task_name == "fishing_task");
    if (!is_hello && !is_fishing) {
        last_error_ = "Unknown task type: " + task_name;
        return false;
    }

    try {
        WindowHandler::get_game_HWND();
    } catch (const WindowException&) {
        last_error_ = "未找到游戏窗口，请先打开游戏。";
        return false;
    }

    if (is_hello) {
        current_task_ = std::make_unique<HelloTask>(task_name);
    } else {
        current_task_ = std::make_unique<FishingTask>(task_name);
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

std::string TaskManager::getLastError() const {
    return last_error_;
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
