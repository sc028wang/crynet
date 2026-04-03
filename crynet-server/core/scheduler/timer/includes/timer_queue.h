#pragma once

#include "core/actor/includes/message.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace crynet::core {

/**
 * @cn
 * 定时器编号类型，用于唯一标识一个已注册的超时任务。
 *
 * @en
 * Timer identifier type used to uniquely identify a registered timeout task.
 */
using TimerId = std::uint64_t;

/**
 * @cn
 * 最小超时队列，负责登记延迟任务、取消任务并在到期时投递 `Timer` 消息。
 *
 * @en
 * Minimal timeout queue responsible for registering delayed tasks, cancelling tasks,
 * and delivering `Timer` messages when they expire.
 */
class TimerQueue final {
public:
    /**
     * @cn
     * 调度一个延迟超时任务，并返回分配的定时器编号。
     *
     * @en
     * Schedule a delayed timeout task and return the allocated timer identifier.
     */
    [[nodiscard]] TimerId schedule_after(
        ServiceHandle target_handle,
        SessionId session_id,
        std::chrono::milliseconds delay,
        std::string payload_text = "timeout"
    );

    /**
     * @cn
     * 取消一个尚未触发的超时任务。
     *
     * @en
     * Cancel a timeout task that has not yet been delivered.
     */
    [[nodiscard]] bool cancel(TimerId timer_id) noexcept;

    /**
     * @cn
     * 在给定时间点轮询并返回所有已到期的超时消息。
     *
     * @en
     * Poll and return all timeout messages that have expired by the specified time point.
     */
    [[nodiscard]] std::vector<Message> poll_expired(std::chrono::steady_clock::time_point now);

    /**
     * @cn
     * 返回当前仍处于待触发状态的定时任务数量。
     *
     * @en
     * Return the number of timeout tasks currently pending delivery.
     */
    [[nodiscard]] std::size_t size() const noexcept;

private:
    /**
     * @cn
     * 单个超时任务的内部记录。
     *
     * @en
     * Internal record describing a single timeout task.
     */
    struct Entry final {
        /**
         * @cn
         * 当前超时任务的唯一编号。
         *
         * @en
         * Unique identifier of the current timeout task.
         */
        TimerId timer_id{0};

        /**
         * @cn
         * 超时消息的目标服务句柄。
         *
         * @en
         * Target service handle for the timeout message.
         */
        ServiceHandle target_handle{0};

        /**
         * @cn
         * 关联请求/响应语义的会话编号。
         *
         * @en
         * Session identifier used to preserve request/response semantics.
         */
        SessionId session_id{0};

        /**
         * @cn
         * 任务到期的绝对时间点。
         *
         * @en
         * Absolute time point at which the task expires.
         */
        std::chrono::steady_clock::time_point deadline;

        /**
         * @cn
         * 到期后写入 `Timer` 消息的文本载荷。
         *
         * @en
         * Text payload written into the delivered `Timer` message.
         */
        std::string payload_text;
    };

    /**
     * @cn
     * 下一个待分配的定时器编号。
     *
     * @en
     * Next timer identifier to allocate.
     */
    TimerId m_next_timer_id{1};

    /**
     * @cn
     * 按到期时间维护的待触发超时任务列表。
     *
     * @en
     * Pending timeout-task list maintained in deadline order.
     */
    std::vector<Entry> m_entries;
};

}  // namespace crynet::core