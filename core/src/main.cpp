#include "basic/service_app.h"
#include "basic/charset.hpp"

int main()
{
    initialize_console_for_utf8();
    ServiceApp app;
    app.run();
    return 0;
}