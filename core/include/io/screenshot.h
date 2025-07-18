#pragma once

#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>

#include <string>
#include <optional>
#include <windows.h>

namespace Screenshot {

    /**
     * @brief 通过窗口句柄(HWND)捕获指定窗口的截图。
     *
     * @param hwnd 要捕获的窗口的句柄。
     * @return cv::Mat 截图的cv::Mat。
     */
    cv::Mat capture(HWND hwnd);

    /**
     * @brief 通过窗口标题捕获指定窗口的截图。
     *
     * @param window_title 要捕获的窗口的标题。
     * @return cv::Mat 截图的cv::Mat。
     */
    cv::Mat capture();

}
