#pragma once

#include "core/actor/includes/message.h"

#include <deque>
#include <optional>
#include <unordered_set>

namespace crynet::core {

/**
 * @cn
 * 全局就绪队列，负责按 FIFO 顺序维护待调度服务句柄，并避免重复入队。
 *
 * @en
 * Global ready queue that keeps runnable service handles in FIFO order while preventing duplicate entries.
 */
class ReadyQueue final {
public:
    /**
     * @cn
     * 将服务句柄压入就绪队列；若已在队列中则忽略重复入队。
     *
     * @en
     * Push a service handle into the ready queue and ignore the request if it is already queued.
     */
    [[nodiscard]] bool push(ServiceHandle handle);

    /**
     * @cn
     * 按 FIFO 顺序弹出下一个就绪服务句柄；若队列为空则返回空值。
     *
     * @en
     * Pop the next runnable service handle in FIFO order and return no value when the queue is empty.
     */
    [[nodiscard]] std::optional<ServiceHandle> pop() noexcept;

    /**
     * @cn
     * 判断指定服务句柄是否已在就绪队列中。
     *
     * @en
     * Check whether the specified service handle is already present in the ready queue.
     */
    [[nodiscard]] bool contains(ServiceHandle handle) const noexcept;

    /**
     * @cn
     * 判断当前就绪队列是否为空。
     *
     * @en
     * Check whether the current ready queue is empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @cn
     * 返回当前就绪队列中的服务数量。
     *
     * @en
     * Return the number of services currently stored in the ready queue.
     */
    [[nodiscard]] std::size_t size() const noexcept;

private:
    /**
     * @cn
     * 维护就绪服务执行顺序的 FIFO 队列。
     *
     * @en
     * FIFO queue preserving runnable-service execution order.
     */
    std::deque<ServiceHandle> m_handles;

    /**
     * @cn
     * 用于检测句柄是否已经在队列中的集合。
     *
     * @en
     * Set used to detect whether a handle is already queued.
     */
    std::unordered_set<ServiceHandle> m_queued_handles;
};

}  // namespace crynet::core