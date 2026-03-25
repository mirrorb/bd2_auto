#include "io/screenshot.h"

#include <chrono>
#include <thread>

#include "basic/exceptions.h"
#include "io/window_handler.h"

namespace {

cv::Mat bitmap_to_bgr(HDC hdc_mem, HBITMAP bitmap, int width, int height) {
    cv::Mat mat(height, width, CV_8UC4);

    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    if (GetDIBits(hdc_mem, bitmap, 0, height, mat.data, reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS) == 0) {
        return {};
    }

    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);
    return bgr;
}

cv::Mat capture_client_area(HWND hwnd, bool activate_window, bool allow_fallback) {
    if (!IsWindow(hwnd)) {
        throw ScreenshotFailedException("game window not found");
    }

    if (IsIconic(hwnd)) {
        if (!activate_window) {
            throw ScreenshotFailedException("window is minimized");
        }
        ShowWindow(hwnd, SW_RESTORE);
    }

    if (activate_window) {
        HWND foreground = GetForegroundWindow();
        DWORD foreground_thread = GetWindowThreadProcessId(foreground, NULL);
        DWORD target_thread = GetWindowThreadProcessId(hwnd, NULL);
        if (foreground_thread != target_thread) {
            AttachThreadInput(foreground_thread, target_thread, TRUE);
        }
        BringWindowToTop(hwnd);
        SetForegroundWindow(hwnd);
        if (foreground_thread != target_thread) {
            AttachThreadInput(foreground_thread, target_thread, FALSE);
        }
    }

    for (int i = 0; i < 10; ++i) {
        RECT ready_rect;
        if (!IsIconic(hwnd) && GetClientRect(hwnd, &ready_rect)) {
            if ((ready_rect.right - ready_rect.left) > 0 && (ready_rect.bottom - ready_rect.top) > 0) {
                break;
            }
        } else if (IsIconic(hwnd)) {
            if (!activate_window) {
                throw ScreenshotFailedException("window is minimized");
            }
            ShowWindow(hwnd, SW_RESTORE);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    RECT client_rect;
    if (!GetClientRect(hwnd, &client_rect)) {
        throw ScreenshotFailedException("failed to query client area");
    }

    const int width = client_rect.right - client_rect.left;
    const int height = client_rect.bottom - client_rect.top;
    if (width <= 0 || height <= 0) {
        throw ScreenshotFailedException("invalid client size");
    }

    HDC hdc_screen = GetDC(hwnd);
    HDC hdc_mem = CreateCompatibleDC(hdc_screen);
    HBITMAP bitmap = CreateCompatibleBitmap(hdc_screen, width, height);
    SelectObject(hdc_mem, bitmap);

    bool ok = PrintWindow(hwnd, hdc_mem, PW_CLIENTONLY | PW_RENDERFULLCONTENT) != 0;
    if (!ok && allow_fallback) {
        POINT client_origin = {0, 0};
        ClientToScreen(hwnd, &client_origin);

        RECT window_rect;
        GetWindowRect(hwnd, &window_rect);

        const int x_offset = client_origin.x - window_rect.left;
        const int y_offset = client_origin.y - window_rect.top;
        ok = BitBlt(hdc_mem, 0, 0, width, height, hdc_screen, x_offset, y_offset, SRCCOPY) != 0;
    }

    cv::Mat result = ok ? bitmap_to_bgr(hdc_mem, bitmap, width, height) : cv::Mat();

    DeleteObject(bitmap);
    DeleteDC(hdc_mem);
    ReleaseDC(hwnd, hdc_screen);

    if (!ok || result.empty()) {
        throw ScreenshotFailedException("failed to capture window image");
    }

    return result;
}

} // namespace

namespace Screenshot {

cv::Mat capture_with_win32() {
    return capture_client_area(WindowHandler::find_game_window(), true, true);
}

cv::Mat capture_with_window_message() {
    return capture_client_area(WindowHandler::find_game_window(), false, false);
}

cv::Mat capture_with_backend(IOBackend::Mode backend) {
    switch (backend) {
    case IOBackend::Mode::WindowMessage:
        return capture_with_window_message();
    case IOBackend::Mode::Win32:
    default:
        return capture_with_win32();
    }
}

} // namespace Screenshot
