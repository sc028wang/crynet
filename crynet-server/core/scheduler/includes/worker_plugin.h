#pragma once

#include "core/actor/includes/actor_system.h"

#include <cstddef>

namespace crynet::core {

/**
 * @cn
 * worker 运行插件接口，用于按需注入监控、观测或其他附加行为。
 *
 * @en
 * Worker-runtime plugin interface used to inject monitoring, observability,
 * or other optional behaviors on demand.
 */
class WorkerPlugin {
public:
    /**
     * @cn
     * 虚析构函数，保证插件通过基类指针销毁时行为正确。
     *
     * @en
     * Virtual destructor ensuring correct behavior when a plugin is destroyed through a base pointer.
     */
    virtual ~WorkerPlugin() = default;

    /**
     * @cn
     * 判断插件是否请求当前 worker 停止继续执行。
     *
     * @en
     * Check whether the plugin requests the current worker to stop further execution.
     */
    [[nodiscard]] virtual bool stop_requested(std::size_t worker_id) const noexcept;

    /**
     * @cn
     * 在消息分发开始前通知插件当前路由信息。
     *
     * @en
     * Notify the plugin of the current route before message dispatch begins.
     */
    virtual void on_dispatch_begin(std::size_t worker_id, const DispatchTrace& trace) noexcept;

    /**
     * @cn
     * 在当前 worker 完成一轮非空分发后通知插件。
     *
     * @en
     * Notify the plugin after the current worker completes one non-idle dispatch turn.
     */
    virtual void on_dispatch_end(std::size_t worker_id) noexcept;

    /**
     * @cn
     * 在当前 worker 进入空闲轮次时通知插件。
     *
     * @en
     * Notify the plugin when the current worker enters an idle turn.
     */
    virtual void on_idle(std::size_t worker_id) noexcept;
};

}  // namespace crynet::core