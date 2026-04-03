#pragma once

#include "core/scheduler/includes/worker_plugin.h"
#include "core/scheduler/includes/worker_loop.h"
#include "core/scheduler/includes/worker_monitor.h"

#include <chrono>
#include <cstddef>
#include <vector>

namespace crynet::core {

/**
 * @cn
 * 单 worker 运行器，负责把 worker loop 与 monitor 状态流转连接起来。
 *
 * @en
 * Single-worker runner that connects the worker loop with monitor-state transitions.
 */
class WorkerRunner final {
public:
    /**
     * @cn
     * 创建一个不附带任何插件的 worker 运行器。
     *
     * @en
     * Create a worker runner without any attached plugins.
     */
    WorkerRunner(ActorSystem& system, std::size_t worker_id) noexcept;

    /**
     * @cn
     * 创建一个绑定到指定系统、monitor 与 worker 编号的运行器。
     *
     * @en
     * Create a runner bound to the specified system, monitor, and worker id.
     */
    WorkerRunner(ActorSystem& system, WorkerMonitor& monitor, std::size_t worker_id) noexcept;

    /**
     * @cn
     * 向当前 worker 运行器添加一个可选插件。
     *
     * @en
     * Add one optional plugin to the current worker runner.
     */
    void add_plugin(WorkerPlugin& plugin) noexcept;

    /**
     * @cn
     * 返回当前挂载的插件数量。
     *
     * @en
     * Return the number of plugins currently attached.
     */
    [[nodiscard]] std::size_t plugin_count() const noexcept;

    /**
     * @cn
     * 执行一轮 worker，自动维护 sleep/wake/busy/quit 状态。
     *
     * @en
     * Execute one worker turn while automatically maintaining sleep, wake, busy, and quit state.
     */
    [[nodiscard]] WorkerLoopTurn run_once(
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now(),
        std::size_t max_messages_per_service = 1
    );

    /**
     * @cn
     * 持续执行直到空闲、达到轮次上限或 monitor 请求退出，返回累计分发数量。
     *
     * @en
     * Continue executing until idle, the cycle limit is reached, or the monitor requests quit,
     * and return the cumulative dispatch count.
     */
    [[nodiscard]] std::size_t run_until_idle(
        std::size_t max_cycles = 64,
        std::size_t max_messages_per_service = 1
    );

private:
    /**
     * @cn
     * 当前 worker 的循环执行器。
     *
     * @en
     * Loop executor used by the current worker.
     */
    WorkerLoop m_loop;

    /**
     * @cn
     * 当前 worker 的编号。
     *
     * @en
     * Identifier of the current worker.
     */
    std::size_t m_worker_id{0};

    /**
     * @cn
     * 当前 worker 附带的可选插件列表。
     *
     * @en
     * List of optional plugins attached to the current worker.
     */
    std::vector<WorkerPlugin*> m_plugins;
};

}  // namespace crynet::core