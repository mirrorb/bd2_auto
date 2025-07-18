#include "basic/logger.h"

Logger::Logger(std::function<void(const json&)> sender)
    : sender_(std::move(sender)) {}

void Logger::info(const std::string& message) const {
    if (sender_) {
        sender_({
            {"type", "log"},
            {"level", "info"},
            {"message", message}
        });
    }
}

void Logger::warn(const std::string& message) const {
    if (sender_) {
        sender_({
            {"type", "log"},
            {"level", "warn"},
            {"message", message}
        });
    }
}

void Logger::error(const std::string& message) const {
    if (sender_) {
        sender_({
            {"type", "log"},
            {"level", "error"},
            {"message", message}
        });
    }
}
