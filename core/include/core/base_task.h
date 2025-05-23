#ifndef BASE_TASK_H
#define BASE_TASK_H

#include <string>
#include <functional>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class BaseTask {
public:
    virtual ~BaseTask() = default;

    virtual std::string getTaskName() const = 0;
    virtual void start(const json& params, std::function<void(const json&)> progress_callback) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual json getStatus() const = 0;
};

#endif // BASE_TASK_H