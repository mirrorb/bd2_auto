#ifndef CHARSET_HPP
#define CHARSET_HPP

#ifdef _WIN32
#include <windows.h>
#endif
inline void initialize_console_for_utf8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    // 在 POSIX 系统 (Linux, macOS) 上:
    // 通常不需要特别做什么
#endif // _WIN32
}
#endif // CHARSET_HPP