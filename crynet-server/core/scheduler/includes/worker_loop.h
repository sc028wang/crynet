#pragma once

#include "core/actor/includes/actor_system.h"

#include <chrono>
#include <cstddef>

namespace crynet::core {

/**
 * @cn
 * 单轮 worker 执行结果，区分定时器注入数量与实际分发数量。
 *
 * @en
 * Result of one worker turn, separating injected timer messages from actually dispatched messages.
 */
struct WorkerLoopTurn final {
    /**
     * @cn
     * 本轮成功注入系统的超时消息数量。
     *
     * @en
     * Number of timeout messages injected into the system during this turn.
     */
    std::size_t injected_timer_messages{0};

    /**
     * @cn
     * 本轮实际执行的消息数量。
     *
     * @en
     * Number of messages actually dispatched during this turn.
     */
    std::size_t dispatched_messages{0};

    /**
     * @cn
     * 判断当前轮次是否没有任何工作发生。
     *
     * @en
     * Check whether no work was performed during the current turn.
     */
    [[nodiscard]] bool idle() const noexcept {
        return injected_timer_messages == 0 && dispatched_messages == 0;
    }
};

/**
 * @cn
 * 最小 worker 执行循环，负责把时间轮轮询与 Actor 调度收束到统一入口。
 *
 * @en
 * Minimal worker execution loop that folds timer polling and actor dispatch into one entry point.
 */
class WorkerLoop final {
public:
    /**
     * @cn
     * 绑定到指定 Actor 系统，供后续循环驱动使用。
     *
     * @en
     * Bind the loop to the specified actor system for subsequent execution.
     */
    explicit WorkerLoop(ActorSystem& system) noexcept;

    /**
     * @cn
     * 执行一轮 worker 循环，先轮询超时，再分发一批消息。
     *
     * @en
     * Execute one worker turn by polling timers first and then dispatching one message batch.
     */
    [[nodiscard]] WorkerLoopTurn run_once(
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now(),
        std::size_t max_messages_per_service = 1,
        DispatchObserver observer = {}
    );

    /**
     * @cn
     * 持续执行直到系统空闲、达到轮次上限或收到停止请求，返回累计分发数量。
     *
     * @en
     * Continue executing until the system becomes idle, the cycle limit is reached,
     * or a stop request is observed, and return the cumulative dispatch count.
     */
    [[nodiscard]] std::size_t run_until_idle(
        std::size_t max_cycles = 64,
        std::size_t max_messages_per_service = 1
    );

    /**
     * @cn
     * 请求当前 worker 循环停止继续执行。
     *
     * @en
     * Request the current worker loop to stop further execution.
     */
    void request_stop() noexcept;

    /**
     * @cn
     * 清除停止请求，使 worker 循环可以再次运行。
     *
     * @en
     * Clear a stop request so the worker loop can run again.
     */
    void reset_stop() noexcept;

    /**
     * @cn
     * 返回当前是否已经收到停止请求。
     *
     * @en
     * Return whether a stop request has been observed.
     */
    [[nodiscard]] bool stop_requested() const noexcept;

private:
    /**
     * @cn
     * 被当前 worker 循环驱动的 Actor 系统实例。
     *
     * @en
     * Actor-system instance driven by the current worker loop.
     */
    ActorSystem& m_system;

    /**
     * @cn
     * 是否请求停止后续循环。
     *
     * @en
     * Whether later loop iterations should stop.
     */
    bool m_stop_requested{false};
};

}  // namespace crynet::core