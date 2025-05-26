#ifndef SERVICE_APP_H
#define SERVICE_APP_H

#include "task_manager.h"
#include <atomic>

class ServiceApp {
private:
    TaskManager task_manager_;
    std::atomic<bool> shutdown_requested_ {false}; // 用于线程安全地请求关闭

    // 处理从stdin接收到的单个命令
    void processCommand(const json& j_request);

public:
    ServiceApp();
    void run();
};

#endif // SERVICE_APP_H