#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <optional>

using json = nlohmann::json;

/**
 * @brief 静态工具类，用于创建和发送 JSON-RPC 2.0 消息。
 *
 * 此类处理两种主要的通信模式：
 * 1. 请求/响应 (Request/Response): 回复客户端的特定请求（成功或失败）。
 * 2. 通知 (Notification): 向客户端主动推送分类的异步消息。
 *
 * 所有消息都通过 stdout 发送。
 */
class JsonRpc {
public:
    //=========================================================================
    // 请求/响应方法
    //=========================================================================

    /**
     * @brief 解析输入的字符串，验证其是否为有效的JSON-RPC请求。
     * @param line 从 stdin 读取的一行字符串。
     * @return 如果是有效请求，则返回包含该请求json的optional。
     *         如果是无效JSON或无效请求，则自动发送错误响应并返回std::nullopt。
     */
    static std::optional<json> parseRequest(const std::string& line);

    /**
     * @brief 发送一个成功的 JSON-RPC 2.0 响应。
     * @param request 客户端的原始请求对象，用于提取 'id'。
     * @param result  包含成功结果的数据，将被封装在响应的 'result' 字段中。
     */
    static void sendSuccessResponse(const json& request, const json& result);

    /**
     * @brief 发送一个错误的 JSON-RPC 2.0 响应。
     * @param request 客户端的原始请求对象，用于提取 'id'。
     * @param error_message 描述性的错误信息。
     */
    static void sendErrorResponse(const json& request, const std::string& error_message);


    //=========================================================================
    // 服务器推送通知 (Server-pushed Notification) 方法
    //=========================================================================

    /**
     * @brief 发送一个 'info' 级别的服务器推送通知。
     * @param method  通知的分类方法名 (例如 "task/log", "system/update")。
     * @param message 人类可读的描述性消息。
     * @param payload (可选) 包含与事件相关的额外结构化数据。
     */
    static void sendInfo(const std::string& method, const std::string& message, const std::optional<json>& payload = std::nullopt);

    /**
     * @brief 发送一个 'warn' 级别的服务器推送通知。
     * @param method  通知的分类方法名。
     * @param message 人类可读的描述性消息。
     * @param payload (可选) 包含与事件相关的额外结构化数据。
     */
    static void sendWarn(const std::string& method, const std::string& message, const std::optional<json>& payload = std::nullopt);

    /**
     * @brief 发送一个 'error' 级别的服务器推送通知。
     * @param method  通知的分类方法名。
     * @param message 人类可读的描述性消息。
     * @param payload (可选) 包含与事件相关的额外结构化数据。
     */
    static void sendError(const std::string& method, const std::string& message, const std::optional<json>& payload = std::nullopt);

    enum class MessageLevel { Info, Warn, Error };

private:
    // 内部实现细节
    static void sendMessage(const std::string& method, MessageLevel level, const std::string& message, const std::optional<json>& payload);
    static std::string levelToString(MessageLevel level);
    static json createResponse(const json& request, const std::optional<json>& result, const std::optional<json>& error);
    static void send(const json& j);
};
