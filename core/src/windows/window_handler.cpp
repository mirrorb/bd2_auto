#include <windows.h>

void rest_game_window() {
    const wchar_t* windowTitle = L"BrownDust II";

    // 定义新的位置和尺寸
    int newX = 0;
    int newY = 0;
    int newWidth = 1280;
    int newHeight = 720;

    // 获取窗口句柄
    HWND hwnd = FindWindowW(NULL, windowTitle);

    if (hwnd == NULL) {
        // "未找到游戏窗口"
    }

    BOOL result = SetWindowPos(hwnd, HWND_TOPMOST, newX, newY, newWidth, newHeight, 0);

    if (!result) {
        // "错误：重设大小并置顶窗口失败，错误代码: " << GetLastError() 
    }

    // 成功将窗口重设大小()并置顶
}
