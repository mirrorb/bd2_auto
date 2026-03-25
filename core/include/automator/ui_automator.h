#pragma once

#include <optional>

#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>

#include <meta/generated_ui.h>

#include "io/mouse_handler.h"

namespace UIAutomator {

inline void click_in_rect(const cv::Rect& target_rect, IOBackend::Mode backend = IOBackend::Mode::WindowMessage, bool instant_move = true) {
    MouseHandler::click_in_rect_with_backend(target_rect, backend, instant_move);
}

inline void drag(
    const cv::Rect& start_rect,
    const cv::Rect& end_rect,
    IOBackend::Mode backend = IOBackend::Mode::WindowMessage,
    bool instant_move = false
) {
    MouseHandler::drag_with_backend(start_rect, end_rect, backend, instant_move);
}

bool verify(const cv::Mat& screen, const UILayouts::Metadata& layout, double confidence = 0.9);

bool verify_click(
    const cv::Mat& screen,
    const UILayouts::Metadata& layout,
    double confidence = 0.9,
    IOBackend::Mode backend = IOBackend::Mode::WindowMessage,
    bool instant_move = true
);

std::optional<cv::Rect> find(const cv::Mat& screen, const UITemplates::Metadata& template_, double confidence = 0.9);

bool find_click(
    const cv::Mat& screen,
    const UITemplates::Metadata& template_,
    double confidence = 0.9,
    IOBackend::Mode backend = IOBackend::Mode::WindowMessage,
    bool instant_move = true
);

} // namespace UIAutomator
