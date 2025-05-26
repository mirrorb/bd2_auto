#include "basic/json_rpc_handler.h"
#include <iostream>

std::optional<json> JsonRpcHandler::parseRequest(const std::string& line) {
    if (line.empty()) {
        return std::nullopt; // 空行不是有效的JSON请求
    }
    try {
        return json::parse(line);
    } catch (json::parse_error& e) {
        // 构建一个错误JSON对象返回给调用者处理，而不是直接发送
        json j_error;
        j_error["type"] = "response";
        j_error["status"] = "error";
        j_error["error_type"] = "json_parse_error";
        j_error["message"] = "JSON解析错误: " + std::string(e.what()) + " (原始输入: '" + line + "')";
        return j_error;
    }
}

void JsonRpcHandler::sendJsonResponse(const json& j_response) {
    std::cout << j_response.dump() << std::endl; // dump()进行序列化
}

json JsonRpcHandler::createResponse(const json& request,
                                  bool success,
                                  const std::optional<json>& data,
                                  const std::optional<std::string>& error_message) {
    json response;
    response["type"] = "response";

    // 如果原始请求中有request_id，则在响应中也包含它
    if (request.is_object() && request.contains("request_id")) {
        response["request_id"] = request["request_id"];
    }

    response["status"] = success ? "success" : "error";

    if (success) {
        if (data) {
            response["data"] = *data;
        } else {
            response["data"] = nullptr;
        }
    } else {
        if (error_message) {
            response["message"] = *error_message;
        } else {
            response["message"] = "发生未知错误。";
        }
    }
    return response;
}