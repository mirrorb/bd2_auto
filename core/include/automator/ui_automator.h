#pragma once

#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <meta/generated_ui_layout.h>

namespace UIAutomator {

    bool verify(const cv::Mat& screen, const UIMetadata& element, double confidence = 0.9);

    bool try_click(const cv::Mat& screen, const UIMetadata& element, double confidence = 0.9);

}
