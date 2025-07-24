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

namespace {

    // 钩子句柄
    HHOOK g_mouseHook = NULL;

    // 低级鼠标钩子回调函数
    LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION) {
            // 吞掉所有鼠标事件，实现禁用效果
            return 1;
        }
        return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
    }

}

void MouseHandler::enable_mouse_hook() {
    if (g_mouseHook == NULL) {
        g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
    }
}

void MouseHandler::disable_mouse_hook() {
    if (g_mouseHook != NULL) {
        UnhookWindowsHookEx(g_mouseHook);
        g_mouseHook = NULL;
    }
}

/**
 * @brief 在矩形内使用正态分布生成一个随机目标点。坐标为客户区坐标。
 */
cv::Point generate_random_point_in_rect(const cv::Rect& target_rect) {
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
 * @brief 将鼠标从当前位置移动到目标点。
 * @param start_point 起始点。
 * @param end_point 目标点。
 */
void move_mouse_humanlike(const cv::Point& start_point, const cv::Point& end_point) {
    cv::Point2d start_point_d(start_point.x, start_point.y);
    cv::Point2d end_point_d(end_point.x, end_point.y);

    double distance = cv::norm(end_point_d - start_point_d);
    if (distance < 3.0) {
        SetCursorPos(end_point.x, end_point.y);
    } else {
        // 贝塞尔曲线、抖动和延迟逻辑
        cv::Point2d vec = end_point_d - start_point_d;
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<> distrib(0.0, 1.0);
        cv::Point2d perpendicular_vec(-vec.y, vec.x);
        double length = cv::norm(perpendicular_vec);
        if (length > 1e-6) { perpendicular_vec.x /= length; perpendicular_vec.y /= length; }
        double offset_magnitude = std::min(distance * 0.4, 150.0) * (distrib(gen) - 0.5) * 2.0;
        cv::Point2d control_1 = start_point_d + 0.25 * vec + 0.5 * offset_magnitude * perpendicular_vec;
        cv::Point2d control_2 = start_point_d + 0.75 * vec - 0.5 * offset_magnitude * perpendicular_vec;
        int num_steps = static_cast<int>(std::max(25.0, distance / 10.0));
        double total_duration_ms = std::max(150.0, distance * (0.4 + distrib(gen) * 0.2));
        auto ease_in_out_cubic = [](double t){ return t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2; };
        for (int i = 0; i <= num_steps; ++i) {
            double t_eased = ease_in_out_cubic(static_cast<double>(i) / num_steps);
            cv::Point2d p_on_curve = std::pow(1 - t_eased, 3) * start_point_d + 3 * std::pow(1 - t_eased, 2) * t_eased * control_1 + 3 * (1 - t_eased) * std::pow(t_eased, 2) * control_2 + std::pow(t_eased, 3) * end_point_d;
            double jitter_x = (distrib(gen) * 2.0 - 1.0) * 1.5;
            double jitter_y = (distrib(gen) * 2.0 - 1.0) * 1.5;
            cv::Point final_pos(static_cast<int>(p_on_curve.x + jitter_x), static_cast<int>(p_on_curve.y + jitter_y));
            SetCursorPos(final_pos.x, final_pos.y);
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(total_duration_ms / num_steps)));
        }
    }

}

void MouseHandler::click_in_rect(const cv::Rect& target_rect, bool instant_move) {
    MouseHookGuard guard;
    // 1. 获取窗口句柄并进行有效性检查
    HWND hwnd = WindowHandler::get_game_HWND();

    // 在客户区坐标系下生成随机目标点并转换为绝对屏幕坐标
    cv::Point target_client_point = generate_random_point_in_rect(target_rect);
    POINT target_screen_point_p = { target_client_point.x, target_client_point.y };
    ClientToScreen(hwnd, &target_screen_point_p);
    cv::Point target_screen_point(target_screen_point_p.x, target_screen_point_p.y);

    // 执行移动
    if (instant_move) {
        SetCursorPos(target_screen_point.x, target_screen_point.y);
    }
    else {
        POINT current_pos_p;
        GetCursorPos(&current_pos_p);
        cv::Point current_screen_point(current_pos_p.x, current_pos_p.y);
        move_mouse_humanlike(current_screen_point, target_screen_point);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20 + rand() % 30));

    // 执行点击动作
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(40 + rand() % 60)); // 模拟按键按下的时长
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void MouseHandler::drag(const cv::Rect& start_rect, const cv::Rect& end_rect, bool instant_move) {
    // hook鼠标
    MouseHookGuard guard;
    // 获取窗口句柄
    HWND hwnd = WindowHandler::get_game_HWND();
    if (!hwnd || !IsWindow(hwnd)) { return; }
    cv::Point start_client_point = generate_random_point_in_rect(start_rect);
    cv::Point end_client_point = generate_random_point_in_rect(end_rect);
    POINT start_screen_point_p = { start_client_point.x, start_client_point.y };
    // 坐标转换
    ClientToScreen(hwnd, &start_screen_point_p);
    POINT end_screen_point_p = { end_client_point.x, end_client_point.y };
    ClientToScreen(hwnd, &end_screen_point_p);
    cv::Point start_screen_point(start_screen_point_p.x, start_screen_point_p.y);
    cv::Point end_screen_point(end_screen_point_p.x, end_screen_point_p.y);

    // 移动到起点
    if (instant_move) {
        SetCursorPos(start_screen_point.x, start_screen_point.y);
    } else {
        POINT current_pos;
        GetCursorPos(&current_pos);
        cv::Point current_screen_point(current_pos.x, current_pos.y);
        move_mouse_humanlike(current_screen_point, start_screen_point); 
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(80 + rand() % 40));

    // 按下鼠标
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(120 + rand() % 60));

    // 执行拖拽
    if (instant_move) {
        SetCursorPos(end_screen_point.x, end_screen_point.y);
    } else {
        move_mouse_humanlike(start_screen_point, end_screen_point);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(80 + rand() % 40));

    // 释放鼠标
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}
