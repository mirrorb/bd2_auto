#include "basic/threaded_task.h"
#include <utility>
#include <basic/exceptions.h>

ThreadedTask::ThreadedTask(std::string name) : task_name_(std::move(name)) {
    updateStatus("空闲", 0);
}

ThreadedTask::~ThreadedTask() {
    if (is_running_.load()) {
        stop();
    }
    if (task_thread_.joinable()) {
        task_thread_.join();
    }
}

void ThreadedTask::registerStep(StepId step_id, StepAction action) {
    if (step_actions_.find(step_id) == step_actions_.end()) {
        workflow_sequence_.push_back(step_id);
    }
    step_actions_[step_id] = std::move(action);
}

void ThreadedTask::start(const json& params, std::shared_ptr<const Logger> logger) {
    if (is_running_.load()) {
        logger->warn("任务 '" + task_name_ + "' 已经在运行中。");
        return;
    }
    params_ = params;
    logger_ = std::move(logger);
    stop_requested_ = false;

    task_thread_ = std::thread(&ThreadedTask::run, this);
}

void ThreadedTask::stop() {
    stop_requested_ = true;
}

std::string ThreadedTask::getTaskName() const {
    return task_name_;
}

bool ThreadedTask::isRunning() const {
    return is_running_.load();
}

json ThreadedTask::getStatus() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    return task_status_json_;
}

void ThreadedTask::updateStatus(const std::string& status_text, int progress) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    task_status_json_["name"] = task_name_;
    task_status_json_["status"] = status_text;
    if (progress != -1) {
        task_status_json_["progress"] = progress;
    }
}

void ThreadedTask::run() {
    is_running_ = true;
    logger_->info("工作流任务 '" + task_name_ + "' 开始。");

    if (workflow_sequence_.empty()) {
        logger_->warn("任务 '" + task_name_ + "' 没有定义任何工作步骤，直接结束。");
        updateStatus("已完成", 100);
        is_running_ = false;
        return;
    }

    size_t total_steps = workflow_sequence_.size();
    for (size_t i = 0; i < total_steps; ++i) {
        StepId current_step_id = workflow_sequence_[i];

        if (stop_requested_.load()) {
            updateStatus("已取消", static_cast<int>((i * 100.0) / total_steps));
            logger_->warn("任务在步骤 " + std::to_string(current_step_id) + " 前被取消。");
            goto cleanup_and_exit;
        }

        std::string status_msg = "运行中：步骤 " + std::to_string(current_step_id);
        updateStatus(status_msg, static_cast<int>((i * 100.0) / total_steps));

        auto it = step_actions_.find(current_step_id);
        if (it == step_actions_.end() || !(it->second)) {
            logger_->error("步骤 " + std::to_string(current_step_id) + " 未实现！任务失败。");
            updateStatus("失败");
            goto cleanup_and_exit;
        }
        // 执行步骤
        try {
            bool success = (it->second)();
            if (!success) {
                logger_->error("步骤 " + std::to_string(current_step_id) + " 执行失败！任务终止。");
                updateStatus("失败");
                goto cleanup_and_exit;
            }
        } catch (const WindowException& e) {
            logger_->error("步骤 " + std::to_string(current_step_id) + ": " + e.what());
            updateStatus("失败");
            goto cleanup_and_exit;
        } catch (const ScreenshotFailedException& e) {
            logger_->error("步骤 " + std::to_string(current_step_id) + ": " + e.what());
            updateStatus("失败");
            goto cleanup_and_exit;
        } catch(const std::exception& e) {
            logger_->error("步骤 " + std::to_string(current_step_id) + " 发生未知错误。");
            updateStatus("失败");
            goto cleanup_and_exit;
        }

    }

    updateStatus("已完成", 100);
    logger_->info("工作流 '" + task_name_ + "' 所有步骤执行完毕。");

cleanup_and_exit:
    is_running_ = false;
}
