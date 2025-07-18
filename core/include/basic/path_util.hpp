#ifndef PATH_UTIL_HPP
#define PATH_UTIL_HPP

#include <filesystem>
#include <stdexcept>
#include <iostream>

// 平台相关的头文件
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h>
#include <limits.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#include <vector>
#include <stdlib.h>
#endif


/**
 * @brief 获取当前可执行文件所在的目录。
 * @details
 *   该函数是跨平台的，会根据操作系统自动选择合适的 API 来定位可执行文件。
 *   这个路径是绝对路径，可以作为可靠的起点来定位资源文件。
 *   使用了 'inline' 关键字，因此可以将实现直接放在头文件中。
 * @return std::filesystem::path 对象，表示可执行文件所在的目录。
 * @throws std::runtime_error 如果无法确定路径。
 */
inline std::filesystem::path get_executable_directory() {
#ifdef _WIN32
    // Windows 平台实现
    wchar_t path_buffer[MAX_PATH];
    if (GetModuleFileNameW(NULL, path_buffer, MAX_PATH) == 0) {
        throw std::runtime_error("Fatal Error: GetModuleFileNameW failed.");
    }
    return std::filesystem::path(path_buffer).parent_path();
#elif __linux__
    // Linux 平台实现
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count < 0) {
        throw std::runtime_error("Fatal Error: readlink /proc/self/exe failed.");
    }
    return std::filesystem::path(std::string(result, count)).parent_path();
#elif __APPLE__
    // macOS 平台实现
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    if (size == 0) {
        throw std::runtime_error("Fatal Error: _NSGetExecutablePath failed to get buffer size.");
    }
    std::vector<char> path_buffer(size);
    if (_NSGetExecutablePath(path_buffer.data(), &size) != 0) {
        throw std::runtime_error("Fatal Error: _NSGetExecutablePath failed.");
    }
    char* real_path = realpath(path_buffer.data(), NULL);
    if (!real_path) {
        throw std::runtime_error("Fatal Error: realpath failed to resolve symlinks.");
    }
    std::filesystem::path p(real_path);
    free(real_path);
    return p.parent_path();
#else
    #error "Unsupported platform: cannot determine executable path."
#endif
}

#endif // PATH_UTIL_HPP
