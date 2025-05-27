#include "basic/service_app.h"
#include "basic/json_rpc_handler.h"
#include <iostream>

// ServiceApp构造函数，初始化TaskManager并传递回调
ServiceApp::ServiceApp() : task_manager_([this](const json& progress_or_event_json){
    JsonRpcHandler::sendResponse(progress_or_event_json);
}) {}

void ServiceApp::run() {
    std::string line;
    // 主循环，持续读取stdin直到请求关闭
    while (!shutdown_requested_.load()) {
        if (std::getline(std::cin, line)) {
            auto opt_request = JsonRpcHandler::parseRequest(line);

            if (opt_request) {
                const json& request_or_error = *opt_request;
                // 检查返回的JSON是否是parseRequest内部构造的错误响应
                if (request_or_error.is_object() &&
                    request_or_error.contains("type") && request_or_error["type"] == "response" &&
                    request_or_error.contains("status") && request_or_error["status"] == "error" &&
                    request_or_error.contains("error_type") && request_or_error["error_type"] == "json_parse_error")
                {
                    JsonRpcHandler::sendResponse(request_or_error);
                } else {
                    processCommand(request_or_error);
                }
            }
        } else {
            if (std::cin.eof() || std::cin.bad() || std::cin.fail()){
                shutdown_requested_ = true; // 设置关闭标志，退出循环
            }
        }
    }
}

// 处理解析后的JSON命令
void ServiceApp::processCommand(const json& j_request) {
    // 基本验证：命令是否存在
    if (!j_request.is_object() || !j_request.contains("command")) {
        JsonRpcHandler::sendResponse(
            JsonRpcHandler::createResponse(j_request, false, std::nullopt, "请求无效或缺少 'command' 字段。")
        );
        return;
    }

    std::string command = j_request["command"].get<std::string>();
    json response_data;

    if (command == "START_TASK") {
        if (!j_request.contains("task_name") || !j_request["task_name"].is_string()) {
             JsonRpcHandler::sendResponse(
                JsonRpcHandler::createResponse(j_request, false, std::nullopt, "START_TASK 命令缺少有效的 'task_name' 字符串。")
            );
            return;
        }
        std::string task_name = j_request["task_name"].get<std::string>();
        // params是可选的，如果不存在，则传递一个空的json对象
        json params = j_request.contains("params") && j_request["params"].is_object() ? j_request["params"] : json::object();

        if (task_manager_.startTask(task_name, params)) {
            response_data["message"] = "任务 '" + task_name + "' 已成功请求启动。"; // 注意措辞，启动是异步的
            JsonRpcHandler::sendResponse(
                JsonRpcHandler::createResponse(j_request, true, response_data)
            );
        } else {
            JsonRpcHandler::sendResponse(
                JsonRpcHandler::createResponse(j_request, false, std::nullopt, "启动任务 '" + task_name + "' 失败 (详见应用日志)。")
            );
        }
    } else if (command == "STOP_CURRENT_TASK") {
        if (task_manager_.stopCurrentTask()) {
            response_data["message"] = "已发送停止当前活动任务的请求。";
             JsonRpcHandler::sendResponse(
                JsonRpcHandler::createResponse(j_request, true, response_data)
            );
        } else {
             JsonRpcHandler::sendResponse(
                JsonRpcHandler::createResponse(j_request, false, std::nullopt, "停止任务请求失败或当前无活动任务。")
            );
        }
    } else if (command == "GET_STATUS") {
        json manager_status_report = task_manager_.getStatus(); // 获取包含任务状态的报告
        // manager_status_report 结构: {"active_task": {...}|null, "message": "..."}
        JsonRpcHandler::sendResponse(
            JsonRpcHandler::createResponse(j_request, true, manager_status_report) // 将整个报告作为data发送
        );
    } else if (command == "SHUTDOWN") {
        response_data["message"] = "后端服务收到关闭请求，即将关闭。";
        JsonRpcHandler::sendResponse(
             JsonRpcHandler::createResponse(j_request, true, response_data)
        );
        shutdown_requested_ = true; // 设置关闭标志
    } else { // 未知命令
        JsonRpcHandler::sendResponse(
             JsonRpcHandler::createResponse(j_request, false, std::nullopt, "未知命令: '" + command + "'。")
        );
    }
}