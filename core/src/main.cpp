#include "basic/service_app.h"
#include <opencv2/core/utils/logger.hpp>
#include <windows.h>

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    ServiceApp app;
    app.run();
    return 0;
}
