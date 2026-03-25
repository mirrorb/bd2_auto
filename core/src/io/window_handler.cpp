#include <windows.h>

#include <chrono>
#include <optional>
#include <thread>

#include "basic/base_config.h"
#include "basic/exceptions.h"
#include "io/win_helper.h"
#include "io/window_handler.h"

namespace {

void apply_window_style(HWND hwnd) {
    const LONG style = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLong(hwnd, GWL_STYLE, style & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX);
}

void apply_window_backend_mode(HWND hwnd, IOBackend::Mode backend) {
    if (backend == IOBackend::Mode::Win32) {
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
        return;
    }

    SetWindowPos(
        hwnd,
        HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED
    );
}

} // namespace

HWND WindowHandler::find_game_window() {
    static HWND game_hwnd = NULL;

    if (!IsWindow(game_hwnd)) {
        game_hwnd = FindWindowW(NULL, BaseConfig::GAME_WINDOW_TITLE);
    }

    if (game_hwnd == NULL) {
        throw WindowException("game window not found");
    }

    return game_hwnd;
}

void WindowHandler::normalize_game_window(int width, int height, int x, int y, IOBackend::Mode backend) {
    HWND hwnd = find_game_window();
    if (!IsWindow(hwnd)) {
        throw WindowException("game window not found");
    }

    std::optional<ThreadInputAttacher> attacher;
    if (backend == IOBackend::Mode::Win32) {
        attacher.emplace();
    }

    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    apply_window_style(hwnd);

    RECT window_rect = {0, 0, width, height};
    DWORD current_style = GetWindowLong(hwnd, GWL_STYLE);
    DWORD ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);

    auto adjust_for_dpi = reinterpret_cast<BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT)>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "AdjustWindowRectExForDpi")
    );
    auto get_dpi_for_window = reinterpret_cast<UINT(WINAPI*)(HWND)>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow")
    );

    BOOL adjusted = FALSE;
    if (adjust_for_dpi) {
        UINT dpi = get_dpi_for_window ? get_dpi_for_window(hwnd) : 96;
        adjusted = adjust_for_dpi(&window_rect, current_style, GetMenu(hwnd) != NULL, ex_style, dpi);
    } else {
        adjusted = AdjustWindowRectEx(&window_rect, current_style, GetMenu(hwnd) != NULL, ex_style);
    }

    if (!adjusted) {
        throw WindowException("failed to compute target window size");
    }

    const int final_window_width = window_rect.right - window_rect.left;
    const int final_window_height = window_rect.bottom - window_rect.top;
    const bool use_foreground = backend == IOBackend::Mode::Win32;
    const HWND z_order = use_foreground ? HWND_TOPMOST : HWND_NOTOPMOST;
    const UINT show_flags = use_foreground ? SWP_SHOWWINDOW : (SWP_SHOWWINDOW | SWP_NOACTIVATE);

    apply_window_backend_mode(hwnd, backend);

    SetWindowPos(
        hwnd,
        z_order,
        x,
        y,
        final_window_width,
        final_window_height,
        show_flags
    );

    RECT initial_client_rect;
    if (GetClientRect(hwnd, &initial_client_rect)) {
        const int client_width = initial_client_rect.right - initial_client_rect.left;
        const int client_height = initial_client_rect.bottom - initial_client_rect.top;
        const int delta_w = width - client_width;
        const int delta_h = height - client_height;
        if (delta_w != 0 || delta_h != 0) {
            SetWindowPos(
                hwnd,
                z_order,
                x,
                y,
                final_window_width + delta_w,
                final_window_height + delta_h,
                show_flags
            );
        }
    }

    bool success = false;
    for (int i = 0; i < 10; ++i) {
        RECT final_client_rect;
        GetClientRect(hwnd, &final_client_rect);
        if (!IsIconic(hwnd) && final_client_rect.right == width && final_client_rect.bottom == height) {
            success = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!success) {
        RECT final_rect;
        GetClientRect(hwnd, &final_rect);
        throw WindowException(
            "failed to normalize window, final client size=" +
            std::to_string(final_rect.right) + "x" +
            std::to_string(final_rect.bottom)
        );
    }
}

void WindowHandler::sync_game_window_mode(IOBackend::Mode backend) {
    HWND hwnd = find_game_window();
    if (!IsWindow(hwnd)) {
        throw WindowException("game window not found");
    }

    apply_window_style(hwnd);
    apply_window_backend_mode(hwnd, backend);
}
