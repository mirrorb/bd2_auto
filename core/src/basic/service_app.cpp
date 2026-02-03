#include "basic/service_app.h"
#include "basic/json_rpc.h"
#include <iostream>

/**
 * ServiceApp 构造函数
 * 将 TaskManager 的异步回调与 JsonRpc 的通知系统连接
 */
ServiceApp::ServiceApp()
    : task_manager_([this](const json& package) {
        std::string type = package.value("type", "log");
        std::string level = package.value("level", "info");
        std::string message = package.value("message", "");

        if (level == "info") {
            JsonRpc::sendInfo(type, message, package);
        } else if (level == "warn") {
            JsonRpc::sendWarn(type, message, package);
        } else if (level == "error") {
            JsonRpc::sendError(type, message, package);
        }
    })
{}

/**
 * 应用主循环
 * 负责从 stdin 读取输入，并将其分派给请求处理器。
 */
void ServiceApp::run() {
    std::string line;
    while (!shutdown_requested_.load()) {
        if (std::getline(std::cin, line)) {
            if (auto opt_request = JsonRpc::parseRequest(line)) {
                // 如果 opt_request 有值，说明它是一个完全有效的请求，可以直接处理
                processCommand(*opt_request);
            }
            // 如果没有值，说明输入有误，parseRequest已经自动发送了错误响应，这里什么都不用做
        } else {
            // 如果输入流结束或出错，则准备关闭应用
            if (std::cin.eof() || std::cin.bad() || std::cin.fail()){
                shutdown_requested_ = true;
            }
        }
    }
}

/**
 * 请求命令处理器
 * 负责解析有效的JSON-RPC请求并执行相应的操作。
 */
void ServiceApp::processCommand(const json& j_request) {
    const std::string method = j_request.value("method", "");
    const json& params = j_request.value("params", json::object());

    if (method == "task/start") {
        const std::string task_name = params.value("task_name", "");
        if (task_name.empty()) {
            JsonRpc::sendErrorResponse(j_request, "START_TASK 命令缺少 'task_name' 参数。");
            return;
        }

        if (task_manager_.startTask(task_name, params)) {
            JsonRpc::sendSuccessResponse(j_request, {{"message", "任务 '" + task_name + "' 已成功请求启动。"}});
        } else {
            JsonRpc::sendErrorResponse(j_request, "启动任务 '" + task_name + "' 失败 (可能已有任务在运行)。");
        }

    } else if (method == "task/stop") {
        if (task_manager_.stopCurrentTask()) {
            JsonRpc::sendSuccessResponse(j_request, {{"message", "已发送停止当前任务的请求。"}});
        } else {
            JsonRpc::sendErrorResponse(j_request, "停止请求失败或当前无活动任务。");
        }

    } else if (method == "app/getStatus") {
        json manager_status_report = task_manager_.getStatus();
        JsonRpc::sendSuccessResponse(j_request, manager_status_report);

    } else if (method == "app/getTasks") {
        json task_list = task_manager_.getTaskList();
        JsonRpc::sendSuccessResponse(j_request, task_list);

    } else if (method == "app/shutdown") {
        JsonRpc::sendSuccessResponse(j_request, {{"message", "后端服务收到关闭请求，即将关闭。"}});
        shutdown_requested_ = true;

    } else {
        JsonRpc::sendErrorResponse(j_request, "未知方法: '" + method + "'。");
    }
}
