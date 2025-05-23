#include "core/service_app.h"
#ifdef _WIN32
#include <windows.h>
#endif
void initialize_console_for_utf8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    // 在 POSIX 系统 (Linux, macOS) 上:
    // 通常不需要特别做什么
#endif
}
int main()
{
    initialize_console_for_utf8();
    ServiceApp app;
    app.run();
    return 0;
}