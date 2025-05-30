#include "basic/service_app.h"
#include "basic/charset.hpp"
#include <opencv2/core/utils/logger.hpp>

int main()
{
    initialize_console_for_utf8();
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    ServiceApp app;
    app.run();
    return 0;
}
