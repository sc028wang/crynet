#pragma once

#include "core/scheduler/includes/ready_queue.h"

#include <optional>
#include <unordered_set>

namespace crynet::core {

/**
 * @cn
 * 最小 worker 调度器骨架，负责管理就绪服务分发与执行中状态。
 *
 * @en
 * Minimal worker-scheduler skeleton that manages runnable-service dispatch and in-flight execution state.
 */
class WorkerScheduler final {
public:
    /**
     * @cn
     * 将服务标记为可调度；若服务已在执行中则延后到完成后再重新进入队列。
     *
     * @en
     * Mark a service as schedulable; if it is already running, rescheduling is deferred until completion.
     */
    [[nodiscard]] bool mark_ready(ServiceHandle handle);

    /**
     * @cn
     * 获取下一个可执行服务句柄，并将其标记为执行中。
     *
     * @en
     * Acquire the next runnable service handle and mark it as in flight.
     */
    [[nodiscard]] std::optional<ServiceHandle> acquire_next() noexcept;

    /**
     * @cn
     * 标记指定服务本轮执行完成，使其可以再次被调度。
     *
     * @en
     * Mark the specified service as finished for the current turn so it can be scheduled again.
     */
    [[nodiscard]] bool complete(ServiceHandle handle) noexcept;

    /**
     * @cn
     * 判断指定服务当前是否处于执行中状态。
     *
     * @en
     * Check whether the specified service is currently in flight.
     */
    [[nodiscard]] bool is_in_flight(ServiceHandle handle) const noexcept;

    /**
     * @cn
     * 返回就绪队列中待执行服务数量。
     *
     * @en
     * Return the number of runnable services waiting in the ready queue.
     */
    [[nodiscard]] std::size_t pending_count() const noexcept;

    /**
     * @cn
     * 返回当前执行中的服务数量。
     *
     * @en
     * Return the number of services currently in flight.
     */
    [[nodiscard]] std::size_t in_flight_count() const noexcept;

private:
    /**
     * @cn
     * 全局就绪队列，用于按顺序分发服务。
     *
     * @en
     * Global ready queue used to dispatch services in order.
     */
    ReadyQueue m_ready_queue;

    /**
     * @cn
     * 记录当前正在 worker 上执行的服务句柄集合。
     *
     * @en
     * Set of service handles currently executing on workers.
     */
    std::unordered_set<ServiceHandle> m_in_flight_handles;
};

}  // namespace crynet::core