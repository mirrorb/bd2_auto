#pragma once

#include <opencv2/core/types.hpp>

namespace MouseHandler {

    /**
     * @brief 在一个矩形区域内进行一次点击。
     * @param target_rect 目标点击区域。
     */
    void click_in_rect(const cv::Rect& target_rect, bool instant_move = true);

    /**
     * @brief 从一个起始区域拖动到结束区域。
     * @param start_rect 拖动操作的起始区域。
     * @param end_rect 拖动操作的结束区域。
     */
    void drag(const cv::Rect& start_rect, const cv::Rect& end_rect, bool instant_move = false);

    // 用于守卫类的钩子控制接口
    void enable_mouse_hook();
    void disable_mouse_hook();

} // namespace MouseHandler

class MouseHookGuard {
public:
    MouseHookGuard() {
        MouseHandler::enable_mouse_hook();
    }
    ~MouseHookGuard() {
        MouseHandler::disable_mouse_hook();
    }
    // 禁用拷贝
    MouseHookGuard(const MouseHookGuard&) = delete;
    MouseHookGuard& operator=(const MouseHookGuard&) = delete;
};
