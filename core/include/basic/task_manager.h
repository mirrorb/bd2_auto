#pragma once

#include <memory>
#include <functional>
#include "nlohmann/json.hpp"
#include "basic/base_task.h"

using json = nlohmann::json;

class TaskManager {
private:
    std::unique_ptr<BaseTask> current_task_;
    std::shared_ptr<Logger> logger_;

public:
    explicit TaskManager(std::function<void(const json&)> sender);

    bool startTask(const std::string& task_name, const json& params);
    bool stopCurrentTask();
    json getStatus() const;
    json getTaskList() const;
};
