#pragma once

#include <stdexcept>
#include <string>

class ScreenshotFailedException : public std::runtime_error {
public:
    explicit ScreenshotFailedException(const std::string& message)
        : std::runtime_error("截图失败: " + message) {}
};

class WindowException : public std::runtime_error {
public:
    explicit WindowException(const std::string& message)
        : std::runtime_error("重置窗口失败: " + message) {}
};
