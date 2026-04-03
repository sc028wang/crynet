#pragma once

#include <atomic>
#include <cstddef>

namespace crynet::core {

/**
 * @cn
 * 指标系统快照，汇总当前 Actor 系统的关键可观测计数。
 *
 * @en
 * Metrics-system snapshot aggregating key observable counters for the current actor system.
 */
struct MetricsSystemSnapshot final {
    /**
     * @cn
     * 已成功注册的服务数量。
     *
     * @en
     * Number of services successfully registered.
     */
    std::size_t registered_services{0};

    /**
     * @cn
     * 当前所有服务队列中的待处理消息总数。
     *
     * @en
     * Total number of pending messages currently buffered across all service queues.
     */
    std::size_t queued_messages{0};

    /**
     * @cn
     * 当前挂起中的请求会话总数。
     *
     * @en
     * Total number of request sessions currently pending.
     */
    std::size_t pending_sessions{0};

    /**
     * @cn
     * 调度器当前就绪队列深度。
     *
     * @en
     * Current runnable-queue depth in the scheduler.
     */
    std::size_t ready_services{0};

    /**
     * @cn
     * 当前处于执行中的服务数量。
     *
     * @en
     * Number of services currently in flight.
     */
    std::size_t in_flight_services{0};

    /**
     * @cn
     * 累计成功入队的消息数量。
     *
     * @en
     * Cumulative number of messages successfully enqueued.
     */
    std::size_t enqueued_messages{0};

    /**
     * @cn
     * 累计成功执行的消息数量。
     *
     * @en
     * Cumulative number of messages successfully dispatched.
     */
    std::size_t dispatched_messages{0};

    /**
     * @cn
     * 累计成功注入系统的超时消息数量。
     *
     * @en
     * Cumulative number of timeout messages successfully injected into the system.
     */
    std::size_t timer_messages{0};

    /**
     * @cn
     * 累计投递失败或被拒绝的消息数量。
     *
     * @en
     * Cumulative number of messages that failed delivery or were rejected.
     */
    std::size_t dropped_messages{0};

    /**
     * @cn
     * 累计触发的队列 overload 事件数量。
     *
     * @en
     * Cumulative number of queue overload events that have been observed.
     */
    std::size_t queue_overload_events{0};

    /**
     * @cn
     * 观测到的单队列峰值深度。
     *
     * @en
     * Highest single-queue depth observed so far.
     */
    std::size_t peak_queue_depth{0};
};

/**
 * @cn
 * 指标系统累计器，负责维护跨调度轮次的计数数据。
 *
 * @en
 * Metrics-system accumulator responsible for maintaining counters across scheduler turns.
 */
class MetricsSystem final {
public:
    void on_service_registered() noexcept;
    void on_message_enqueued() noexcept;
    void on_message_dispatched() noexcept;
    void on_timer_message() noexcept;
    void on_message_dropped() noexcept;
    void on_queue_overload() noexcept;
    void observe_queue_depth(std::size_t depth) noexcept;

    /**
     * @cn
     * 返回累计计数快照。
     *
     * @en
     * Return a snapshot of cumulative counters.
     */
    [[nodiscard]] MetricsSystemSnapshot snapshot() const noexcept;

private:
    std::atomic<std::size_t> m_registered_services{0};
    std::atomic<std::size_t> m_enqueued_messages{0};
    std::atomic<std::size_t> m_dispatched_messages{0};
    std::atomic<std::size_t> m_timer_messages{0};
    std::atomic<std::size_t> m_dropped_messages{0};
    std::atomic<std::size_t> m_queue_overload_events{0};
    std::atomic<std::size_t> m_peak_queue_depth{0};
};

}  // namespace crynet::core