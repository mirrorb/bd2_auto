#pragma once

#include <windows.h>

#include "io/backend.h"

namespace WindowHandler {

    /**
     * @brief 获取已缓存的游戏窗口句柄。
     *
     * 如果是第一次调用，它会内部执行FindWindowW来查找并缓存句柄。
     * 后续调用将直接返回缓存的句柄。
     *
     * @return 游戏窗口的HWND。
     * @throw WindowException 如果找不到窗口，则抛出异常。
     */
    HWND find_game_window();

    /**
     * @brief 重置并激活游戏窗口，设置其客户区大小和屏幕位置。
     * 
     * @param width 目标客户区的宽度 (例如: 1280)
     * @param height 目标客户区的高度 (例如: 720)
     * @param x 窗口左上角的X坐标 (例如: 0)
     * @param y 窗口左上角的Y坐标 (例如: 0)
     */
    void normalize_game_window(int width, int height, int x, int y, IOBackend::Mode backend = IOBackend::Mode::WindowMessage);

    void sync_game_window_mode(IOBackend::Mode backend);

    inline HWND get_game_HWND() {
        return find_game_window();
    }

    inline void reset_game_window(int width, int height, int x, int y, IOBackend::Mode backend = IOBackend::Mode::WindowMessage) {
        normalize_game_window(width, height, x, y, backend);
    }

}
