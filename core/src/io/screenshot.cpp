#include <iostream>
#include "io/screenshot.h"
#include "basic/base_config.h"
#include "basic/exceptions.h"
#include "io/window_handler.h"

namespace Screenshot {

    cv::Mat capture() {
        HWND hwnd = WindowHandler::get_game_HWND();
        if (!IsWindow(hwnd)) {
            throw ScreenshotFailedException("未找到游戏窗口，请先打开游戏。");
        }

        RECT client_rect;
        if (!GetClientRect(hwnd, &client_rect)) {
            throw ScreenshotFailedException("获取窗口区域时发生错误");
        }

        int width = client_rect.right - client_rect.left;
        int height = client_rect.bottom - client_rect.top;

        if (width == 0 || height == 0) {
            throw ScreenshotFailedException("获取窗口客户区时发生错误");
        }

        HDC hdc_screen = GetDC(hwnd);
        HDC hdc_mem = CreateCompatibleDC(hdc_screen);
        HBITMAP hbmp = CreateCompatibleBitmap(hdc_screen, width, height);
        SelectObject(hdc_mem, hbmp);

        if (!PrintWindow(hwnd, hdc_mem, PW_CLIENTONLY | PW_RENDERFULLCONTENT)) {

            // 计算客户区相对于窗口左上角的偏移量
            POINT client_origin = {0, 0};
            ClientToScreen(hwnd, &client_origin); // 将客户区的(0,0)转换为屏幕坐标

            RECT window_rect;
            GetWindowRect(hwnd, &window_rect); // 获取整个窗口的屏幕坐标
            
            // 计算偏移
            int x_offset = client_origin.x - window_rect.left;
            int y_offset = client_origin.y - window_rect.top;

            // 从计算出的客户区偏移点开始复制
            BitBlt(hdc_mem, 0, 0, width, height, hdc_screen, x_offset, y_offset, SRCCOPY);
        }
        
        cv::Mat mat(height, width, CV_8UC4);
        BITMAPINFOHEADER bi;
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = width;
        bi.biHeight = -height;
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        
        GetDIBits(hdc_mem, hbmp, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        // 清理资源
        DeleteObject(hbmp);
        DeleteDC(hdc_mem);
        ReleaseDC(hwnd, hdc_screen);

        cv::Mat result_bgr;
        cv::cvtColor(mat, result_bgr, cv::COLOR_BGRA2BGR);

        return result_bgr;
    }

}
