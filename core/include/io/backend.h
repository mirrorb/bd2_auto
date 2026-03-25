#pragma once

#include <string_view>

namespace IOBackend {

enum class Mode {
    Win32,
    WindowMessage,
};

inline Mode from_string(std::string_view value) {
    if (value == "window_message" || value == "window-message" || value == "message") {
        return Mode::WindowMessage;
    }
    return Mode::WindowMessage;
}

inline const char* to_string(Mode mode) {
    switch (mode) {
    case Mode::WindowMessage:
        return "window_message";
    case Mode::Win32:
    default:
        return "win32";
    }
}

} // namespace IOBackend
