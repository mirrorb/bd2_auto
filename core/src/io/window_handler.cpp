#include <windows.h>
#include <thread>
#include <chrono>
#include <iostream>
#include "basic/base_config.h"
#include "basic/exceptions.h"
#include "io/win_helper.h"
#include <io/window_handler.h>

HWND WindowHandler::get_game_HWND() {
    // 使用静态局部变量来存储窗口句柄。
    // 这个变量只会在getHWND函数第一次被调用时被初始化一次。
    static HWND game_hwnd = NULL;

    // 如果句柄尚未缓存，或者缓存的句柄对应的窗口已失效，
    // 则尝试重新查找。
    if (!IsWindow(game_hwnd)) {
        game_hwnd = FindWindowW(NULL, BaseConfig::GAME_WINDOW_TITLE);
    }

    // 如果最终句柄仍然无效，则抛出异常
    if (game_hwnd == NULL) {
        throw WindowException("未能找到或初始化目标窗口句柄。");
    }

    return game_hwnd;
}

void WindowHandler::reset_game_window(int width, int height, int x, int y) {
    HWND hwnd = get_game_HWND();
    if (!IsWindow(hwnd)) {
        throw WindowException("未找到游戏窗口，请先打开游戏。");
    }

    // 自动附加和分离线程输入，以确保我们可以操作前台窗口
    ThreadInputAttacher attacher;

    // 如果窗口最小化，则恢复它
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
        // 等待一小段时间让窗口完成恢复动画
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    // 移除可能阻止调整大小的窗口样式
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    // 移除边框和最大化按钮，让尺寸更可控
    SetWindowLong(hwnd, GWL_STYLE, style & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX);
    
    // 计算实现目标客户区所需的整个窗口的尺寸
    RECT windowRect = {0, 0, width, height};
    DWORD currentStyle = GetWindowLong(hwnd, GWL_STYLE);
    DWORD exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    auto adjust_for_dpi = reinterpret_cast<BOOL (WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT)>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi")
    );
    auto get_dpi_for_window = reinterpret_cast<UINT (WINAPI*)(HWND)>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow")
    );

    BOOL adjusted = FALSE;
    if (adjust_for_dpi) {
        UINT dpi = get_dpi_for_window ? get_dpi_for_window(hwnd) : 96;
        adjusted = adjust_for_dpi(&windowRect, currentStyle, GetMenu(hwnd) != NULL, exStyle, dpi);
    } else {
        adjusted = AdjustWindowRectEx(&windowRect, currentStyle, GetMenu(hwnd) != NULL, exStyle);
    }

    if (!adjusted) {
        DWORD errorCode = GetLastError(); // 获取详细的 WinAPI 错误码
        std::string errorMsg = "无法根据目标客户区计算窗口尺寸。WinAPI 错误码: " + std::to_string(errorCode);

        // 将错误码转换为可读的字符串
        LPSTR messageBuffer = nullptr;
        size_t size = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, 
            errorCode, 
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
            (LPSTR)&messageBuffer, 
            0, 
            NULL
        );

        if (size > 0) {
            // 将系统错误信息附加到我们的消息后面
            errorMsg += " (" + std::string(messageBuffer, size) + ")";
            LocalFree(messageBuffer); // 释放由 FormatMessageA 分配的内存
        }

        throw WindowException(errorMsg);
    }

    // 计算出的窗口总宽度和总高度
    int finalWindowWidth = windowRect.right - windowRect.left;
    int finalWindowHeight = windowRect.bottom - windowRect.top;

    // 将窗口带到前台并设置其位置和大小
    BringWindowToTop(hwnd);
    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED
    );
    SetForegroundWindow(hwnd);
    // 移动和调整大小
    SetWindowPos(
        hwnd, 
        HWND_TOPMOST, // 设置置顶
        x, 
        y, 
        finalWindowWidth, 
        finalWindowHeight, 
        SWP_SHOWWINDOW
    );
    // 验证操作是否成功
    bool success = false;
    for (int i = 0; i < 10; ++i) { // 最多重试10次，总计500ms
        RECT finalClientRect;
        GetClientRect(hwnd, &finalClientRect);
        if (!IsIconic(hwnd) && finalClientRect.right == width && finalClientRect.bottom == height) {
            success = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!success) {
        RECT finalRect;
        GetClientRect(hwnd, &finalRect);
        std::string errMsg = "重置游戏窗口失败。";
        if(IsIconic(hwnd)) errMsg += " 窗口仍处于最小化状态。";
        else errMsg += " 最终客户区尺寸为 " + std::to_string(finalRect.right) + "x" + std::to_string(finalRect.bottom);
        
        throw WindowException(errMsg);
    }
}

