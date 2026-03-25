#define NOMINMAX

#include "io/mouse_handler.h"

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <random>
#include <thread>

#include "io/window_handler.h"

namespace {

HHOOK g_mouse_hook = NULL;

struct MessageTarget {
    HWND hwnd = NULL;
    cv::Point point;
};

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        return 1;
    }
    return CallNextHookEx(g_mouse_hook, nCode, wParam, lParam);
}

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

void move_mouse_humanlike(const cv::Point& start_point, const cv::Point& end_point) {
    cv::Point2d start_point_d(start_point.x, start_point.y);
    cv::Point2d end_point_d(end_point.x, end_point.y);

    const double distance = cv::norm(end_point_d - start_point_d);
    if (distance < 3.0) {
        SetCursorPos(end_point.x, end_point.y);
        return;
    }

    cv::Point2d vec = end_point_d - start_point_d;
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> distrib(0.0, 1.0);
    cv::Point2d perpendicular_vec(-vec.y, vec.x);
    const double length = cv::norm(perpendicular_vec);
    if (length > 1e-6) {
        perpendicular_vec.x /= length;
        perpendicular_vec.y /= length;
    }

    const double offset_magnitude = std::min(distance * 0.4, 150.0) * (distrib(gen) - 0.5) * 2.0;
    const cv::Point2d control_1 = start_point_d + 0.25 * vec + 0.5 * offset_magnitude * perpendicular_vec;
    const cv::Point2d control_2 = start_point_d + 0.75 * vec - 0.5 * offset_magnitude * perpendicular_vec;

    const int num_steps = static_cast<int>(std::max(25.0, distance / 10.0));
    const double total_duration_ms = std::max(150.0, distance * (0.4 + distrib(gen) * 0.2));
    const auto ease_in_out_cubic = [](double t) {
        return t < 0.5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
    };

    for (int i = 0; i <= num_steps; ++i) {
        const double t_eased = ease_in_out_cubic(static_cast<double>(i) / num_steps);
        const cv::Point2d point_on_curve =
            std::pow(1 - t_eased, 3) * start_point_d +
            3 * std::pow(1 - t_eased, 2) * t_eased * control_1 +
            3 * (1 - t_eased) * std::pow(t_eased, 2) * control_2 +
            std::pow(t_eased, 3) * end_point_d;

        const double jitter_x = (distrib(gen) * 2.0 - 1.0) * 1.5;
        const double jitter_y = (distrib(gen) * 2.0 - 1.0) * 1.5;
        const cv::Point final_pos(static_cast<int>(point_on_curve.x + jitter_x), static_cast<int>(point_on_curve.y + jitter_y));
        SetCursorPos(final_pos.x, final_pos.y);
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(total_duration_ms / num_steps)));
    }
}

LPARAM make_client_lparam(const cv::Point& point) {
    return LPARAM((static_cast<int>(point.y) << 16) | (static_cast<int>(point.x) & 0xFFFF));
}

void post_window_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    PostMessageA(hwnd, message, wparam, lparam);
}

void send_cancel_mode(HWND hwnd) {
    HWND target_hwnd = GetParent(hwnd);
    if (!target_hwnd) {
        target_hwnd = hwnd;
    }
    post_window_message(target_hwnd, WM_CANCELMODE, WPARAM(0), LPARAM(0));
}

float ease_in_out(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    }
    return 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

cv::Point map_point_between_windows(HWND from, HWND to, const cv::Point& point) {
    POINT mapped = {point.x, point.y};
    MapWindowPoints(from, to, &mapped, 1);
    return cv::Point(mapped.x, mapped.y);
}

MessageTarget resolve_message_target(HWND root_hwnd, const cv::Point& root_client_point) {
    MessageTarget target{root_hwnd, root_client_point};
    HWND current_hwnd = root_hwnd;
    cv::Point current_point = root_client_point;

    while (current_hwnd && IsWindow(current_hwnd)) {
        POINT probe = {current_point.x, current_point.y};
        HWND child = ChildWindowFromPointEx(
            current_hwnd,
            probe,
            CWP_SKIPDISABLED | CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT
        );

        if (!child || child == current_hwnd) {
            break;
        }

        current_point = map_point_between_windows(current_hwnd, child, current_point);
        current_hwnd = child;
        target.hwnd = current_hwnd;
        target.point = current_point;
    }

    return target;
}

void drag_via_window_messages(HWND hwnd, const cv::Point& start_point, const cv::Point& end_point, bool instant_move) {
    const auto total_duration = std::chrono::milliseconds(instant_move ? 40 : 140);
    const LPARAM down_pos = make_client_lparam(start_point);

    send_cancel_mode(hwnd);
    post_window_message(hwnd, WM_MOUSEMOVE, WPARAM(0), down_pos);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    post_window_message(hwnd, WM_LBUTTONDOWN, WPARAM(MK_LBUTTON), down_pos);

    cv::Point last_point = start_point;
    const auto started_at = std::chrono::steady_clock::now();

    while (true) {
        const auto elapsed = std::chrono::steady_clock::now() - started_at;
        if (elapsed >= total_duration) {
            break;
        }

        const float t = std::chrono::duration<float>(elapsed).count() / std::chrono::duration<float>(total_duration).count();
        const float eased = ease_in_out(std::clamp(t, 0.0f, 1.0f));
        const cv::Point current(
            start_point.x + static_cast<int>(std::round((end_point.x - start_point.x) * eased)),
            start_point.y + static_cast<int>(std::round((end_point.y - start_point.y) * eased))
        );

        if (current != last_point) {
            post_window_message(hwnd, WM_MOUSEMOVE, WPARAM(MK_LBUTTON), make_client_lparam(current));
            last_point = current;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    const LPARAM up_pos = make_client_lparam(end_point);
    post_window_message(hwnd, WM_MOUSEMOVE, WPARAM(MK_LBUTTON), up_pos);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    post_window_message(hwnd, WM_LBUTTONUP, WPARAM(0), up_pos);
}

} // namespace

void MouseHandler::enable_mouse_hook() {
    if (g_mouse_hook == NULL) {
        g_mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
    }
}

void MouseHandler::disable_mouse_hook() {
    if (g_mouse_hook != NULL) {
        UnhookWindowsHookEx(g_mouse_hook);
        g_mouse_hook = NULL;
    }
}

void MouseHandler::click_in_rect_with_win32(const cv::Rect& target_rect, bool instant_move) {
    MouseHookGuard guard;
    HWND hwnd = WindowHandler::find_game_window();

    const cv::Point target_client_point = generate_random_point_in_rect(target_rect);
    POINT target_screen_point_raw = {target_client_point.x, target_client_point.y};
    ClientToScreen(hwnd, &target_screen_point_raw);
    const cv::Point target_screen_point(target_screen_point_raw.x, target_screen_point_raw.y);

    if (instant_move) {
        SetCursorPos(target_screen_point.x, target_screen_point.y);
    } else {
        POINT current_pos_raw;
        GetCursorPos(&current_pos_raw);
        move_mouse_humanlike(cv::Point(current_pos_raw.x, current_pos_raw.y), target_screen_point);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20 + rand() % 30));
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(40 + rand() % 60));
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void MouseHandler::click_in_rect_with_window_message(const cv::Rect& target_rect, bool instant_move) {
    HWND root_hwnd = WindowHandler::find_game_window();
    const cv::Point root_client_point = generate_random_point_in_rect(target_rect);
    const MessageTarget target = resolve_message_target(root_hwnd, root_client_point);
    const LPARAM pos = make_client_lparam(target.point);

    send_cancel_mode(target.hwnd);
    post_window_message(target.hwnd, WM_MOUSEMOVE, WPARAM(0), pos);
    std::this_thread::sleep_for(std::chrono::milliseconds(instant_move ? 6 : 12));
    post_window_message(target.hwnd, WM_LBUTTONDOWN, WPARAM(MK_LBUTTON), pos);
    std::this_thread::sleep_for(std::chrono::milliseconds(instant_move ? 6 : 12));
    post_window_message(target.hwnd, WM_LBUTTONUP, WPARAM(0), pos);
}

void MouseHandler::click_in_rect_with_backend(const cv::Rect& target_rect, IOBackend::Mode backend, bool instant_move) {
    switch (backend) {
    case IOBackend::Mode::WindowMessage:
        click_in_rect_with_window_message(target_rect, instant_move);
        break;
    case IOBackend::Mode::Win32:
    default:
        click_in_rect_with_win32(target_rect, instant_move);
        break;
    }
}

void MouseHandler::drag_with_win32(const cv::Rect& start_rect, const cv::Rect& end_rect, bool instant_move) {
    MouseHookGuard guard;
    HWND hwnd = WindowHandler::find_game_window();
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }

    const cv::Point start_client_point = generate_random_point_in_rect(start_rect);
    const cv::Point end_client_point = generate_random_point_in_rect(end_rect);

    POINT start_screen_point_raw = {start_client_point.x, start_client_point.y};
    ClientToScreen(hwnd, &start_screen_point_raw);
    POINT end_screen_point_raw = {end_client_point.x, end_client_point.y};
    ClientToScreen(hwnd, &end_screen_point_raw);

    const cv::Point start_screen_point(start_screen_point_raw.x, start_screen_point_raw.y);
    const cv::Point end_screen_point(end_screen_point_raw.x, end_screen_point_raw.y);

    if (instant_move) {
        SetCursorPos(start_screen_point.x, start_screen_point.y);
    } else {
        POINT current_pos_raw;
        GetCursorPos(&current_pos_raw);
        move_mouse_humanlike(cv::Point(current_pos_raw.x, current_pos_raw.y), start_screen_point);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(80 + rand() % 40));
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(120 + rand() % 60));

    if (instant_move) {
        SetCursorPos(end_screen_point.x, end_screen_point.y);
    } else {
        move_mouse_humanlike(start_screen_point, end_screen_point);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(80 + rand() % 40));
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void MouseHandler::drag_with_window_message(const cv::Rect& start_rect, const cv::Rect& end_rect, bool instant_move) {
    HWND root_hwnd = WindowHandler::find_game_window();
    if (!root_hwnd || !IsWindow(root_hwnd)) {
        return;
    }

    const cv::Point root_start = generate_random_point_in_rect(start_rect);
    const cv::Point root_end = generate_random_point_in_rect(end_rect);
    const MessageTarget target = resolve_message_target(root_hwnd, root_start);
    const cv::Point end_point = map_point_between_windows(root_hwnd, target.hwnd, root_end);

    drag_via_window_messages(
        target.hwnd,
        target.point,
        end_point,
        instant_move
    );
}

void MouseHandler::drag_with_backend(const cv::Rect& start_rect, const cv::Rect& end_rect, IOBackend::Mode backend, bool instant_move) {
    switch (backend) {
    case IOBackend::Mode::WindowMessage:
        drag_with_window_message(start_rect, end_rect, instant_move);
        break;
    case IOBackend::Mode::Win32:
    default:
        drag_with_win32(start_rect, end_rect, instant_move);
        break;
    }
}
