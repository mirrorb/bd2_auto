#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <vector>
#include <string>
#include "basic/charset.hpp"

// --- 配置参数 ---
const std::string SCENE_IMAGE_PATH = "game_screenshot.png"; // 游戏截图
const std::string OBJECT_IMAGE_PATH = "element_template.png"; // 你要找的元素模板
const int MIN_MATCH_COUNT = 4;        // 至少需要多少个好的匹配点才能认为找到了对象
const float RATIO_TEST_THRESH = 0.75f; // Lowe's ratio test a (knnMatch)



// 辅助函数：等比例缩放图像（基于宽度）
cv::Mat resizeImage(const cv::Mat& src, int targetWidth, int interpolation = cv::INTER_AREA) {
    if (targetWidth <= 0 || src.empty()) {
        return src;
    }
    double ratio = static_cast<double>(targetWidth) / src.cols;
    int targetHeight = static_cast<int>(src.rows * ratio);
    if (targetHeight <= 0) {
        return src; // Avoid invalid size
    }
    cv::Mat resized_img;
    cv::resize(src, resized_img, cv::Size(targetWidth, targetHeight), 0, 0, interpolation);
    return resized_img;
}


int main() {
    initialize_console_for_utf8();
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

    // 转换为灰度图
    cv::Mat img_object_gray, img_scene_gray;
    cv::cvtColor(img_object_bgr, img_object_gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(img_scene_bgr, img_scene_gray, cv::COLOR_BGR2GRAY);

    // --- 2. 初始化特征检测器和描述子提取器 ---
    cv::Ptr<cv::Feature2D> detector_descriptor = cv::SIFT::create();
    if (!detector_descriptor) {
        std::cerr << "错误: 无法创建特征检测器!" << std::endl;
        return -1;
    }

    // --- 3. 检测关键点并计算描述子 ---
    std::vector<cv::KeyPoint> keypoints_object, keypoints_scene;
    cv::Mat descriptors_object, descriptors_scene;

    std::cout << "检测对象特征点..." << std::endl;
    detector_descriptor->detectAndCompute(img_object_gray, cv::noArray(), keypoints_object, descriptors_object);
    std::cout << "检测场景特征点..." << std::endl;
    detector_descriptor->detectAndCompute(img_scene_gray, cv::noArray(), keypoints_scene, descriptors_scene);

    if (keypoints_object.empty() || descriptors_object.empty()) {
        std::cerr << "错误: 模板图中未找到特征点。" << std::endl;
        return -1;
    }
    if (keypoints_scene.empty() || descriptors_scene.empty()) {
        std::cerr << "错误: 场景图中未找到特征点。" << std::endl;
        return -1;
    }
     std::cout << "模板特征点数: " << keypoints_object.size() << std::endl;
     std::cout << "场景特征点数: " << keypoints_scene.size() << std::endl;

    // --- 4. 匹配描述子 ---
    cv::Ptr<cv::DescriptorMatcher> matcher;
    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);

    std::vector<std::vector<cv::DMatch>> knn_matches;
    std::cout << "匹配特征点..." << std::endl;
    // 使用 k-NN 匹配，k=1 表示为每个对象描述子只找场景中最近的1个匹配
    matcher->knnMatch(descriptors_object, descriptors_scene, knn_matches, 1); // 注意这里 k 可以设置为 1

    // --- 5. 获取所有最佳匹配（不进行Lowe's Ratio Test） ---
    std::vector<cv::DMatch> good_matches; // 重命名为 all_best_matches 或类似的可能更清晰，但沿用 good_matches 也可以

    std::cout << "获取所有最佳匹配点..." << std::endl;
    for (size_t i = 0; i < knn_matches.size(); ++i) {
        if (!knn_matches[i].empty()) { // 确保至少有一个匹配项
            good_matches.push_back(knn_matches[i][0]);
        }
    }
    std::cout << "获取到的最佳匹配点数: " << good_matches.size() << std::endl;

    // --- 6. 如果有足够多的匹配（这里可以根据您的需求调整 MIN_MATCH_COUNT 的意义）---
    cv::Mat img_matches; // 用于显示匹配结果的图像
    cv::Mat output_image = img_scene_bgr.clone();

    // 即使没有Ratio Test，MIN_MATCH_COUNT 仍然可以用来判断是否有足够的点进行后续处理（如单应性矩阵）
    if (good_matches.size() >= MIN_MATCH_COUNT) {
        std::cout << "有足够多的匹配点 (" << good_matches.size() << ")，尝试定位对象..." << std::endl;
        std::vector<cv::Point2f> obj_pts;
        std::vector<cv::Point2f> scene_pts;

        for (size_t i = 0; i < good_matches.size(); ++i) {
            // 获取关键点的位置
            obj_pts.push_back(keypoints_object[good_matches[i].queryIdx].pt);
            scene_pts.push_back(keypoints_scene[good_matches[i].trainIdx].pt);
        }

        // 计算单应性矩阵 (Homography)
        // RANSAC 用于处理匹配中的一些外点 (outliers)
        // 当匹配质量不高时（因为没有Ratio Test筛选），RANSAC的作用尤为重要
        cv::Mat H = cv::findHomography(obj_pts, scene_pts, cv::RANSAC, 5.0); // 5.0 是 RANSAC 的重投影误差阈值

        if (!H.empty()) {
            // 获取对象图像的角点
            std::vector<cv::Point2f> obj_corners(4);
            obj_corners[0] = cv::Point2f(0, 0);
            obj_corners[1] = cv::Point2f(static_cast<float>(img_object_gray.cols), 0);
            obj_corners[2] = cv::Point2f(static_cast<float>(img_object_gray.cols), static_cast<float>(img_object_gray.rows));
            obj_corners[3] = cv::Point2f(0, static_cast<float>(img_object_gray.rows));

            std::vector<cv::Point2f> scene_corners(4);
            // 使用单应性矩阵变换对象角点到场景图像中的位置
            cv::perspectiveTransform(obj_corners, scene_corners, H);

            // 绘制边界框
            cv::line(output_image, scene_corners[0], scene_corners[1], cv::Scalar(0, 255, 0), 4);
            cv::line(output_image, scene_corners[1], scene_corners[2], cv::Scalar(0, 255, 0), 4);
            cv::line(output_image, scene_corners[2], scene_corners[3], cv::Scalar(0, 255, 0), 4);
            cv::line(output_image, scene_corners[3], scene_corners[0], cv::Scalar(0, 255, 0), 4);

            std::cout << "对象已在场景中定位并标记。" << std::endl;
        } else {
            std::cout << "无法计算单应性矩阵 (Homography)。" << std::endl;
        }
        // 绘制匹配线 (可选，但有助于调试)
        cv::drawMatches(img_object_bgr, keypoints_object, img_scene_bgr, keypoints_scene,
                        good_matches, img_matches, cv::Scalar::all(-1), cv::Scalar::all(-1),
                        std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

    } else {
        std::cout << "未找到足够多的匹配点 (" << good_matches.size() << "/" << MIN_MATCH_COUNT << ")" << std::endl;
        // 即使没有足够匹配，也画出所有找到的匹配（如果想看的话）
         cv::drawMatches(img_object_bgr, keypoints_object, img_scene_bgr, keypoints_scene,
                        good_matches, img_matches, cv::Scalar::all(-1), cv::Scalar::all(-1),
                        std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
    }


    // --- 7. 显示结果 ---
    if (!img_matches.empty()) {
        cv::imshow("Good Matches & Object detection", resizeImage(img_matches, 1200));
    } else {
         cv::imshow("Output Image (No/Few Matches)", resizeImage(output_image, 1000));
    }
    cv::imshow("Detected Object in Scene", resizeImage(output_image, 1000));
    cv::imshow("Object Template", resizeImage(img_object_bgr, 200));


    std::cout << "\n按任意键退出..." << std::endl;
    cv::waitKey(0);
    cv::destroyAllWindows();

    return 0;
}