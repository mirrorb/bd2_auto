#include "basic/json_rpc.h"
#include <iostream>

//=========================================================================
// 请求/响应 方法实现
//=========================================================================

std::optional<json> JsonRpc::parseRequest(const std::string& line) {
    json request;
    try {
        request = json::parse(line);
    } catch (...) {
        // JSON 格式本身错误，无法解析
        json error_response;
        error_response["jsonrpc"] = "2.0";
        error_response["id"] = nullptr;
        error_response["error"]["code"] = -32700; // Parse error
        error_response["error"]["message"] = "JSON parse error: " + line;
        send(error_response);
        return std::nullopt;
    }

    // 验证是否是符合规范的请求
    if (!request.is_object() || !request.contains("jsonrpc") || !request.contains("method") || !request.contains("id")) {
        json error_response;
        error_response["jsonrpc"] = "2.0";
        if (request.is_object()) {
            error_response["id"] = request.value("id", nullptr);
        } else {
            error_response["id"] = nullptr;
        }
        error_response["error"]["code"] = -32600; // Invalid Request
        error_response["error"]["message"] = "Invalid JSON-RPC request object.";
        send(error_response);
        return std::nullopt;
    }

    return request;
}

void JsonRpc::sendSuccessResponse(const json& request, const json& result) {
    send(createResponse(request, result, std::nullopt));
}

void JsonRpc::sendErrorResponse(const json& request, const std::string& error_message) {
    json error_obj;
    error_obj["code"] = -32000; // 使用一个通用的服务器错误码
    error_obj["message"] = error_message;

    send(createResponse(request, std::nullopt, error_obj));
}


//=========================================================================
// 服务器推送通知方法实现
//=========================================================================

void JsonRpc::sendInfo(const std::string& method, const std::string& message, const std::optional<json>& payload) {
    sendMessage(method, MessageLevel::Info, message, payload);
}

void JsonRpc::sendWarn(const std::string& method, const std::string& message, const std::optional<json>& payload) {
    sendMessage(method, MessageLevel::Warn, message, payload);
}

void JsonRpc::sendError(const std::string& method, const std::string& message, const std::optional<json>& payload) {
    sendMessage(method, MessageLevel::Error, message, payload);
}


//=========================================================================
// 私有辅助函数实现
//=========================================================================

/**
 * @brief 将 MessageLevel 枚举转换为其字符串表示形式。
 */
std::string JsonRpc::levelToString(MessageLevel level) {
    switch (level) {
        case MessageLevel::Info:  return "info";
        case MessageLevel::Warn:  return "warn";
        case MessageLevel::Error: return "error";
    }
    return "info";
}

/**
 * @brief 构造并发送一个完整的服务器推送通知。
 */
void JsonRpc::sendMessage(const std::string& method, MessageLevel level, const std::string& message, const std::optional<json>& payload) {
    json notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = method;

    json params;
    params["level"] = levelToString(level);
    params["message"] = message;

    if (payload.has_value()) {
        params["payload"] = *payload;
    }
    notification["params"] = params;

    send(notification);
}

/**
 * @brief 标准的 JSON-RPC 响应。
 * 该函数确保所有响应都包含 'jsonrpc' 版本和正确的 'id'。
 */
json JsonRpc::createResponse(const json& request,
                             const std::optional<json>& result,
                             const std::optional<json>& error) {
    json response;
    response["jsonrpc"] = "2.0";

    response["id"] = request.contains("id") ? request["id"] : nullptr;

    if (result.has_value()) {
        response["result"] = *result;
    } else if (error.has_value()) {
        response["error"] = *error;
    } else {
        response["result"] = nullptr;
    }

    return response;
}

/**
 * @brief 将最终的 JSON 对象序列化为字符串并发送到标准输出。
 * 这是所有消息发送的最终出口。
 */
void JsonRpc::send(const json& j) {
    std::cout << j.dump() << std::endl;
}
