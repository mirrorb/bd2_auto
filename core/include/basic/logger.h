#pragma once

#include <functional>
#include <string>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class Logger {
private:
    std::function<void(const json&)> sender_;

public:
    explicit Logger(std::function<void(const json&)> sender);

    void info(const std::string& message) const;
    void warn(const std::string& message) const;
    void error(const std::string& message) const;
};
