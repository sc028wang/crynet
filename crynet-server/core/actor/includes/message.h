#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace crynet::core {

/**
 * @cn
 * 服务句柄类型，用于唯一标识运行时中的服务实例。
 *
 * @en
 * Service-handle type used to uniquely identify a service instance in the runtime.
 */
using ServiceHandle = std::uint64_t;

/**
 * @cn
 * 会话编号类型，用于关联请求与响应。
 *
 * @en
 * Session identifier type used to correlate requests and responses.
 */
using SessionId = std::uint64_t;

/**
 * @cn
 * 运行时消息类型枚举，用于区分请求、响应、事件和系统消息。
 *
 * @en
 * Runtime message-type enumeration for requests, responses, events and system traffic.
 */
enum class MessageType : std::uint8_t {
    Invalid = 0,
    Request,
    Response,
    Event,
    Timer,
    System,
};

/**
 * @cn
 * 返回消息类型的稳定文本名称，方便日志和调试输出。
 *
 * @en
 * Return a stable text name for a message type for logging and debugging.
 */
[[nodiscard]] std::string_view message_type_name(MessageType type) noexcept;

/**
 * @cn
 * 最小运行时消息对象，封装来源、目标、会话以及二进制载荷。
 *
 * @en
 * Minimal runtime message object holding source, destination, session and binary payload.
 */
class Message final {
public:
    /**
     * @cn
     * 构造一条空消息，默认处于 `Invalid` 状态。
     *
     * @en
     * Construct an empty message that starts in the `Invalid` state.
     */
    Message() = default;

    /**
     * @cn
     * 使用完整元数据和二进制载荷构造一条运行时消息。
     *
     * @en
     * Construct a runtime message from full metadata and a binary payload.
     */
    Message(
        MessageType type,
        ServiceHandle source_handle,
        ServiceHandle target_handle,
        SessionId session_id,
        std::vector<std::byte> payload = {}
    ) noexcept;

    /**
     * @cn
     * 使用 UTF-8 文本快速构造一条消息，用于 T1 阶段的最小链路验证。
     *
     * @en
     * Build a message from UTF-8 text for T1-stage minimum flow validation.
     */
    [[nodiscard]] static Message from_text(
        MessageType type,
        ServiceHandle source_handle,
        ServiceHandle target_handle,
        SessionId session_id,
        std::string_view text_payload
    );

    /**
     * @cn
     * 返回当前消息类型。
     *
     * @en
     * Return the current message type.
     */
    [[nodiscard]] MessageType type() const noexcept;

    /**
     * @cn
     * 返回消息发送方句柄。
     *
     * @en
     * Return the source-service handle.
     */
    [[nodiscard]] ServiceHandle source_handle() const noexcept;

    /**
     * @cn
     * 返回消息目标服务句柄。
     *
     * @en
     * Return the target-service handle.
     */
    [[nodiscard]] ServiceHandle target_handle() const noexcept;

    /**
     * @cn
     * 返回消息所属会话编号。
     *
     * @en
     * Return the session identifier bound to the message.
     */
    [[nodiscard]] SessionId session_id() const noexcept;

    /**
     * @cn
     * 判断消息是否不包含任何载荷字节。
     *
     * @en
     * Check whether the message contains no payload bytes.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @cn
     * 返回当前载荷的字节数。
     *
     * @en
     * Return the current payload size in bytes.
     */
    [[nodiscard]] std::size_t payload_size() const noexcept;

    /**
     * @cn
     * 以只读字节数组引用返回当前载荷。
     *
     * @en
     * Return the current payload as a read-only byte-array reference.
     */
    [[nodiscard]] const std::vector<std::byte>& payload() const noexcept;

    /**
     * @cn
     * 将载荷按 UTF-8 文本视图解释并返回。
     *
     * @en
     * Interpret the payload as a UTF-8 text view and return it.
     */
    [[nodiscard]] std::string_view text_payload() const noexcept;

private:
    /**
     * @cn
     * 当前消息的类型标签。
     *
     * @en
     * Type tag carried by the current message.
     */
    MessageType m_type{MessageType::Invalid};

    /**
     * @cn
     * 发送方服务句柄。
     *
     * @en
     * Source-service handle.
     */
    ServiceHandle m_source_handle{0};

    /**
     * @cn
     * 目标服务句柄。
     *
     * @en
     * Target-service handle.
     */
    ServiceHandle m_target_handle{0};

    /**
     * @cn
     * 请求与响应关联使用的会话编号。
     *
     * @en
     * Session identifier used to correlate requests and responses.
     */
    SessionId m_session_id{0};

    /**
     * @cn
     * 按原始字节保存的消息载荷。
     *
     * @en
     * Message payload stored as raw bytes.
     */
    std::vector<std::byte> m_payload;
};

}  // namespace crynet::core