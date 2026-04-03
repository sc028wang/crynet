#pragma once

#include "core/actor/includes/message.h"

#include <chrono>
#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <array>
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

    /**
     * @cn
     * 取消某个目标服务名下的所有定时任务，并返回移除数量。
     *
     * @en
     * Cancel all timeout tasks owned by the specified target service and return the removal count.
     */
    [[nodiscard]] std::size_t cancel_for_target(ServiceHandle target_handle) noexcept;

private:
    /**
     * @cn
     * 近端时间轮位移位数，对应 256 个近端槽位。
     *
     * @en
     * Shift width for the near wheel, yielding 256 near buckets.
     */
    static constexpr std::uint64_t k_near_shift = 8;

    /**
     * @cn
     * 近端时间轮槽位数量。
     *
     * @en
     * Bucket count of the near wheel.
     */
    static constexpr std::uint64_t k_near_size = 1ULL << k_near_shift;

    /**
     * @cn
     * 层级时间轮单层位移位数，对应 64 个槽位。
     *
     * @en
     * Shift width for each level wheel, yielding 64 buckets per level.
     */
    static constexpr std::uint64_t k_level_shift = 6;

    /**
     * @cn
     * 单层时间轮槽位数量。
     *
     * @en
     * Bucket count of each level wheel.
     */
    static constexpr std::uint64_t k_level_size = 1ULL << k_level_shift;

    /**
     * @cn
     * 时间轮层级数量。
     *
     * @en
     * Number of level wheels.
     */
    static constexpr std::size_t k_level_count = 4;

    /**
     * @cn
     * 近端时间轮掩码。
     *
     * @en
     * Bit mask used by the near wheel.
     */
    static constexpr std::uint64_t k_near_mask = k_near_size - 1;

    /**
     * @cn
     * 层级时间轮掩码。
     *
     * @en
     * Bit mask used by each level wheel.
     */
    static constexpr std::uint64_t k_level_mask = k_level_size - 1;

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
         * 任务到期的逻辑 tick。
         *
         * @en
         * Logical tick at which the task expires.
         */
        std::uint64_t expire_tick{0};

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
     * 单个时间轮槽位的任务链表类型。
     *
     * @en
     * Task-list type stored by one wheel bucket.
     */
    using Bucket = std::list<Entry>;

    /**
     * @cn
     * 将任务插入到合适的时间轮槽位。
     *
     * @en
     * Insert a task into the appropriate timing-wheel bucket.
     */
    void add_entry(Entry entry);

    /**
     * @cn
     * 将高层时间轮某个槽位中的任务下沉到更低层。
     *
     * @en
     * Cascade tasks from one higher-level bucket down into lower wheels.
     */
    void move_bucket(std::size_t level, std::size_t index);

    /**
     * @cn
     * 推进一个逻辑 tick，并完成必要的层级迁移。
     *
     * @en
     * Advance one logical tick and perform any required bucket cascading.
     */
    void advance_tick();

    /**
     * @cn
     * 取出当前近端槽位中的所有到期任务。
     *
     * @en
     * Drain all expired tasks from the current near-wheel bucket.
     */
    void collect_current_bucket(std::vector<Message>& expired_messages);

    /**
     * @cn
     * 在所有时间轮槽位中按定时器编号移除任务。
     *
     * @en
     * Remove a task by timer identifier across all wheel buckets.
     */
    [[nodiscard]] bool remove_by_timer_id(TimerId timer_id) noexcept;

    /**
     * @cn
     * 在所有时间轮槽位中按目标服务移除任务。
     *
     * @en
     * Remove tasks by target service across all wheel buckets.
     */
    [[nodiscard]] std::size_t remove_by_target(ServiceHandle target_handle) noexcept;

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
     * 最近一次推进时间点，对应时间轮的逻辑时间基线。
     *
     * @en
     * Most recently advanced time point used as the logical time baseline of the wheel.
     */
    std::chrono::steady_clock::time_point m_current_point{std::chrono::steady_clock::now()};

    /**
     * @cn
     * 当前时间轮逻辑 tick。
     *
     * @en
     * Current logical tick of the timing wheel.
     */
    std::uint64_t m_current_tick{0};

    /**
     * @cn
     * 当前待触发任务总数。
     *
     * @en
     * Total number of pending tasks.
     */
    std::size_t m_pending_size{0};

    /**
     * @cn
     * 近端时间轮。
     *
     * @en
     * Near timing wheel.
     */
    std::array<Bucket, k_near_size> m_near{};

    /**
     * @cn
     * 多层时间轮。
     *
     * @en
     * Multi-level timing wheels.
     */
    std::array<std::array<Bucket, k_level_size>, k_level_count> m_levels{};
};

}  // namespace crynet::core