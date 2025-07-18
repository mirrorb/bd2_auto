#pragma once

#include <string>
#include <memory>
#include "nlohmann/json.hpp"
#include "logger.h"

using json = nlohmann::json;

class BaseTask {
public:
    virtual ~BaseTask() = default;

    virtual std::string getTaskName() const = 0;
    virtual void start(const json& params, std::shared_ptr<const Logger> logger) = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual json getStatus() const = 0;
};
