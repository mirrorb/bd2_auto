#include <iostream>
#include <vector>
#include <string>
#include <windows.h>

#include "cv/point_matcher.h"

#include <opencv2/core/utils/logger.hpp>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    const std::string SCENE_IMAGE_PATH = "game_screenshot.png";
    const std::string OBJECT_IMAGE_PATH = "element_template.png";

    // --- 1. 加载图片 ---
    std::cout << "加载场景图片: " << SCENE_IMAGE_PATH << std::endl;
    cv::Mat img_scene_bgr = cv::imread(SCENE_IMAGE_PATH, cv::IMREAD_COLOR);
    if (img_scene_bgr.empty()) {
        std::cerr << "错误: 无法加载场景图片 '" << SCENE_IMAGE_PATH << "'" << std::endl;
        return -1;
    }
    std::cout << "加载对象图片: " << OBJECT_IMAGE_PATH << std::endl;
    cv::Mat img_object_bgr = cv::imread(OBJECT_IMAGE_PATH, cv::IMREAD_COLOR);
    if (img_object_bgr.empty()) {
        std::cerr << "错误: 无法加载对象图片 '" << OBJECT_IMAGE_PATH << "'" << std::endl;
        return -1;
    }
    PointMatcher matcher {5.0, 1.5f, 0.2f};
    std::vector<cv::Point2f> points = matcher.get_points(img_scene_bgr, img_object_bgr);
    for (const auto& p : points) {
        std::cout << "(" << int(p.x) << ", " << int(p.y) << ")" << std::endl;
    }
    return 0;
}
