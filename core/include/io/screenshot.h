#pragma once

#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>

#include "io/backend.h"

namespace Screenshot {

cv::Mat capture_with_win32();

cv::Mat capture_with_window_message();

cv::Mat capture_with_backend(IOBackend::Mode backend = IOBackend::Mode::WindowMessage);

inline cv::Mat capture() {
    return capture_with_backend();
}

} // namespace Screenshot
