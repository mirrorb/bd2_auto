#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "base_task.h"
#include <memory>
#include <string>
#include <functional>

class TaskManager {
private:
    std::unique_ptr<BaseTask> current_task_; // 当前活动的任务
    std::function<void(const json&)> global_progress_callback_; // 用于任务向外发送消息的回调

public:
    // 接收一个回调函数用于任务发送消息
    explicit TaskManager(std::function<void(const json&)> progress_callback);

    bool startTask(const std::string& task_name, const json& params);
    bool stopCurrentTask();
    json getStatus() const;
};

#endif // TASK_MANAGER_H