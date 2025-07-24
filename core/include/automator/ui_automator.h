#pragma once

#include <optional>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <meta/generated_ui.h>
#include "io/mouse_handler.h"

namespace UIAutomator {

    using MouseHandler::click_in_rect;
    using MouseHandler::drag;

    bool verify(const cv::Mat& screen, const UILayouts::Metadata& layout, double confidence = 0.9);

    bool verify_click(const cv::Mat& screen, const UILayouts::Metadata& layout, double confidence = 0.9);

    std::optional<cv::Rect> find(const cv::Mat& screen, const UITemplates::Metadata& template_, double confidence = 0.9);

    bool find_click(const cv::Mat& screen, const UITemplates::Metadata& template_, double confidence = 0.9);

}
