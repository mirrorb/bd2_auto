#pragma once

#include <opencv2/core/types.hpp>

#include "io/backend.h"

namespace MouseHandler {

void click_in_rect_with_win32(const cv::Rect& target_rect, bool instant_move = true);

void click_in_rect_with_window_message(const cv::Rect& target_rect, bool instant_move = true);

void click_in_rect_with_backend(const cv::Rect& target_rect, IOBackend::Mode backend = IOBackend::Mode::WindowMessage, bool instant_move = true);

void drag_with_win32(const cv::Rect& start_rect, const cv::Rect& end_rect, bool instant_move = false);

void drag_with_window_message(const cv::Rect& start_rect, const cv::Rect& end_rect, bool instant_move = false);

void drag_with_backend(
    const cv::Rect& start_rect,
    const cv::Rect& end_rect,
    IOBackend::Mode backend = IOBackend::Mode::WindowMessage,
    bool instant_move = false
);

inline void click_in_rect(const cv::Rect& target_rect, bool instant_move = true) {
    click_in_rect_with_backend(target_rect, IOBackend::Mode::WindowMessage, instant_move);
}

inline void drag(const cv::Rect& start_rect, const cv::Rect& end_rect, bool instant_move = false) {
    drag_with_backend(start_rect, end_rect, IOBackend::Mode::WindowMessage, instant_move);
}

void enable_mouse_hook();
void disable_mouse_hook();

} // namespace MouseHandler

class MouseHookGuard {
public:
    MouseHookGuard() {
        MouseHandler::enable_mouse_hook();
    }
    ~MouseHookGuard() {
        MouseHandler::disable_mouse_hook();
    }
    MouseHookGuard(const MouseHookGuard&) = delete;
    MouseHookGuard& operator=(const MouseHookGuard&) = delete;
};
