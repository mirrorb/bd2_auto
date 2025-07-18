#define NOMINMAX

#include "io/mouse_handler.h"
#include <windows.h>
#include <thread>
#include <random>
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>
#include <io/window_handler.h>

/**
 * @brief 在矩形内使用正态分布生成一个随机目标点。坐标为客户区坐标。
 */
cv::Point generate_random_point_in_rect(const cv::Rect& target_rect) {
    // ... (此函数实现与之前完全相同，无需修改) ...
    const double center_x = target_rect.x + target_rect.width / 2.0;
    const double center_y = target_rect.y + target_rect.height / 2.0;
    const double std_dev_x = std::max(1.0, target_rect.width / 6.0);
    const double std_dev_y = std::max(1.0, target_rect.height / 6.0);
    static std::mt19937 gen(std::random_device{}());
    std::normal_distribution<> distrib_x(center_x, std_dev_x);
    std::normal_distribution<> distrib_y(center_y, std_dev_y);
    int click_x = static_cast<int>(distrib_x(gen));
    int click_y = static_cast<int>(distrib_y(gen));
    click_x = std::max(target_rect.x, std::min(target_rect.x + target_rect.width - 1, click_x));
    click_y = std::max(target_rect.y, std::min(target_rect.y + target_rect.height - 1, click_y));
    return cv::Point(click_x, click_y);
}

/**
 * @brief [核心] 以模拟人的轨迹将鼠标从当前位置移动到目标点。
 * @param end_point 目标点的。
 */
void move_mouse_humanlike(const cv::Point& end_point) {
    POINT current_pos;
    GetCursorPos(&current_pos); 

    cv::Point2d start_point(current_pos.x, current_pos.y);
    // 贝塞尔曲线和抖动逻辑
    cv::Point2d end_point_d(end_point.x, end_point.y);
    cv::Point2d vec = end_point_d - start_point;
    double distance = cv::norm(vec);
    if (distance < 3.0) { SetCursorPos(end_point.x, end_point.y); return; }
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> distrib(0.0, 1.0);
    cv::Point2d perpendicular_vec(-vec.y, vec.x);
    double length = std::sqrt(perpendicular_vec.x * perpendicular_vec.x + perpendicular_vec.y * perpendicular_vec.y);
    if (length > 1e-6) { perpendicular_vec.x /= length; perpendicular_vec.y /= length; }
    double offset_magnitude = std::min(distance * 0.4, 150.0) * (distrib(gen) - 0.5) * 2.0;
    cv::Point2d control_1 = start_point + 0.25 * vec + 0.5 * offset_magnitude * perpendicular_vec;
    cv::Point2d control_2 = start_point + 0.75 * vec - 0.5 * offset_magnitude * perpendicular_vec;
    int num_steps = static_cast<int>(std::max(25.0, distance / 10.0));
    double total_duration_ms = std::max(150.0, distance * (0.4 + distrib(gen) * 0.2));
    auto ease_in_out_cubic = [](double t){ return t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2; };
    for (int i = 0; i <= num_steps; ++i) {
        double t_eased = ease_in_out_cubic(static_cast<double>(i) / num_steps);
        cv::Point2d p_on_curve = std::pow(1 - t_eased, 3) * start_point + 3 * std::pow(1 - t_eased, 2) * t_eased * control_1 + 3 * (1 - t_eased) * std::pow(t_eased, 2) * control_2 + std::pow(t_eased, 3) * end_point_d;
        double jitter_x = (distrib(gen) * 2.0 - 1.0) * 1.5;
        double jitter_y = (distrib(gen) * 2.0 - 1.0) * 1.5;
        cv::Point final_pos(static_cast<int>(p_on_curve.x + jitter_x), static_cast<int>(p_on_curve.y + jitter_y));
        SetCursorPos(final_pos.x, final_pos.y);
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(total_duration_ms / num_steps)));
    }
    SetCursorPos(end_point.x, end_point.y);
}


void MouseHandler::click_in_rect(const cv::Rect& target_rect) {
    // 从window_handler获取HWND
    HWND hwnd = WindowHandler::get_game_HWND();

    // 在客户区坐标系下生成一个随机目标点
    cv::Point target_client_point = generate_random_point_in_rect(target_rect);

    // 执行坐标转换
    POINT target_screen_point = { target_client_point.x, target_client_point.y };
    ClientToScreen(hwnd, &target_screen_point);

    // 调用移动函数，传入转换后的屏幕坐标
    move_mouse_humanlike(cv::Point(target_screen_point.x, target_screen_point.y));

    // 执行点击
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(40 + rand() % 60));
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void MouseHandler::drag(const cv::Rect& start_rect, const cv::Rect& end_rect) {
    // 从window_handler获取HWND
    HWND hwnd = WindowHandler::get_game_HWND();

    // 在客户区坐标系下生成起点和终点
    cv::Point start_client_point = generate_random_point_in_rect(start_rect);
    cv::Point end_client_point = generate_random_point_in_rect(end_rect);

    // 对两个点都进行坐标转换
    POINT start_screen_point = { start_client_point.x, start_client_point.y };
    ClientToScreen(hwnd, &start_screen_point);
    POINT end_screen_point = { end_client_point.x, end_client_point.y };
    ClientToScreen(hwnd, &end_screen_point);

    // 执行拖放，所有移动都基于转换后的屏幕坐标
    move_mouse_humanlike(cv::Point(start_screen_point.x, start_screen_point.y));
    std::this_thread::sleep_for(std::chrono::milliseconds(100 + rand() % 50));

    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(150 + rand() % 75));

    move_mouse_humanlike(cv::Point(end_screen_point.x, end_screen_point.y));
    std::this_thread::sleep_for(std::chrono::milliseconds(100 + rand() % 50));

    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}
