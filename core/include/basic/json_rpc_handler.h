#ifndef JSON_RPC_HANDLER_H
#define JSON_RPC_HANDLER_H

#include "nlohmann/json.hpp"
#include <string>
#include <optional>

using json = nlohmann::json;

class JsonRpcHandler {
public:
    // 解析输入的字符串为JSON对象，如果失败则返回包含错误信息的JSON或std::nullopt
    static std::optional<json> parseRequest(const std::string& line);

    // 将JSON对象序列化为字符串并发送到stdout
    static void sendJsonResponse(const json& j_response);

    // 创建一个标准的响应JSON对象
    // @param request: 原始请求的JSON，用于获取request_id (如果存在)
    // @param success: 响应是否成功
    // @param data: (可选) 成功时携带的数据
    // @param error_message: (可选) 失败时携带的错误信息
    static json createResponse(const json& request,
                               bool success,
                               const std::optional<json>& data = std::nullopt,
                               const std::optional<std::string>& error_message = std::nullopt);
};

#endif // JSON_RPC_HANDLER_H