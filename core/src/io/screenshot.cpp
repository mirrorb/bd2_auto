#include <iostream>
#include <thread>
#include <chrono>
#include "io/screenshot.h"
#include "basic/base_config.h"
#include "basic/exceptions.h"
#include "io/window_handler.h"
#include "io/win_helper.h"

namespace Screenshot {

    cv::Mat capture() {
        HWND hwnd = WindowHandler::get_game_HWND();
        if (!IsWindow(hwnd)) {
            throw ScreenshotFailedException("未找到游戏窗口，请先打开游戏。");
        }

        // Ensure the window is foregrounded before capture.
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }
        {
            HWND foreground = GetForegroundWindow();
            DWORD foregroundThreadId = GetWindowThreadProcessId(foreground, NULL);
            DWORD targetThreadId = GetWindowThreadProcessId(hwnd, NULL);
            if (foregroundThreadId != targetThreadId) {
                AttachThreadInput(foregroundThreadId, targetThreadId, TRUE);
            }
            BringWindowToTop(hwnd);
            SetForegroundWindow(hwnd);
            if (foregroundThreadId != targetThreadId) {
                AttachThreadInput(foregroundThreadId, targetThreadId, FALSE);
            }
        }
        // Wait briefly for the window to be restored and ready.
        for (int i = 0; i < 10; ++i) {
            if (!IsIconic(hwnd)) {
                RECT readyRect;
                if (GetClientRect(hwnd, &readyRect)) {
                    if ((readyRect.right - readyRect.left) > 0 && (readyRect.bottom - readyRect.top) > 0) {
                        break;
                    }
                }
            } else {
                ShowWindow(hwnd, SW_RESTORE);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

        auto capture_once = [&](cv::Mat& out) -> bool {
            HDC hdc_screen = GetDC(hwnd);
            HDC hdc_mem = CreateCompatibleDC(hdc_screen);
            HBITMAP hbmp = CreateCompatibleBitmap(hdc_screen, width, height);
            SelectObject(hdc_mem, hbmp);

            bool ok = true;
            if (!PrintWindow(hwnd, hdc_mem, PW_CLIENTONLY | PW_RENDERFULLCONTENT)) {
                POINT client_origin = {0, 0};
                ClientToScreen(hwnd, &client_origin);

                RECT window_rect;
                GetWindowRect(hwnd, &window_rect);
                
                int x_offset = client_origin.x - window_rect.left;
                int y_offset = client_origin.y - window_rect.top;

                ok = BitBlt(hdc_mem, 0, 0, width, height, hdc_screen, x_offset, y_offset, SRCCOPY) != 0;
            }
            
            cv::Mat mat(height, width, CV_8UC4);
            BITMAPINFOHEADER bi;
            bi.biSize = sizeof(BITMAPINFOHEADER);
            bi.biWidth = width;
            bi.biHeight = -height;
            bi.biPlanes = 1;
            bi.biBitCount = 32;
            bi.biCompression = BI_RGB;
            
            if (GetDIBits(hdc_mem, hbmp, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS) == 0) {
                ok = false;
            }

            DeleteObject(hbmp);
            DeleteDC(hdc_mem);
            ReleaseDC(hwnd, hdc_screen);

            if (!ok || mat.empty()) {
                out.release();
                return false;
            }

            cv::cvtColor(mat, out, cv::COLOR_BGRA2BGR);
            return !out.empty();
        };

        cv::Mat result_bgr;
        if (!capture_once(result_bgr)) {
            if (IsIconic(hwnd)) {
                ShowWindow(hwnd, SW_RESTORE);
            }
            BringWindowToTop(hwnd);
            SetForegroundWindow(hwnd);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            if (!capture_once(result_bgr)) {
                throw ScreenshotFailedException("截图失败：窗口未成功置顶或图像为空");
            }
        }

        return result_bgr;
    }

}
