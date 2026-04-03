#pragma once

#include "core/actor/includes/message.h"

#include <optional>
#include <unordered_map>

namespace crynet::core {

/**
 * @cn
 * 最小会话管理器，负责分配请求会话并校验响应回流关系。
 *
 * @en
 * Minimal session manager responsible for allocating request sessions and validating response routing.
 */
class SessionManager final {
public:
    /**
     * @cn
     * 为一次请求分配新的会话编号，并记录调用方与被调用方。
     *
     * @en
     * Allocate a new session identifier for a request and record caller/callee metadata.
     */
    [[nodiscard]] SessionId begin(ServiceHandle caller_handle, ServiceHandle callee_handle) noexcept;

    /**
     * @cn
     * 根据响应方句柄查询会话对应的调用方句柄；若不匹配则返回空值。
     *
     * @en
     * Resolve the caller handle for a session using the responder handle and return no value on mismatch.
     */
    [[nodiscard]] std::optional<ServiceHandle> lookup_requester(
        SessionId session_id,
        ServiceHandle responder_handle
    ) const noexcept;

    /**
     * @cn
     * 在响应成功投递后关闭指定会话。
     *
     * @en
     * Close the specified session after a response has been delivered successfully.
     */
    [[nodiscard]] bool complete(SessionId session_id, ServiceHandle responder_handle) noexcept;

    /**
     * @cn
     * 判断指定会话当前是否仍处于挂起状态。
     *
     * @en
     * Check whether the specified session is still pending.
     */
    [[nodiscard]] bool contains(SessionId session_id) const noexcept;

    /**
     * @cn
     * 返回当前挂起中的会话数量。
     *
     * @en
     * Return the number of sessions currently pending.
     */
    [[nodiscard]] std::size_t size() const noexcept;

private:
    /**
     * @cn
     * 单个挂起会话的内部记录。
     *
     * @en
     * Internal record describing one pending session.
     */
    struct Entry final {
        /**
         * @cn
         * 请求发起方服务句柄。
         *
         * @en
         * Service handle of the request initiator.
         */
        ServiceHandle caller_handle{0};

        /**
         * @cn
         * 请求接收方服务句柄。
         *
         * @en
         * Service handle of the request receiver.
         */
        ServiceHandle callee_handle{0};
    };

    /**
     * @cn
     * 下一个待分配的会话编号。
     *
     * @en
     * Next session identifier to allocate.
     */
    SessionId m_next_session_id{1};

    /**
     * @cn
     * 会话编号到挂起会话信息的映射表。
     *
     * @en
     * Mapping table from session identifiers to pending-session metadata.
     */
    std::unordered_map<SessionId, Entry> m_entries;
};

}  // namespace crynet::core