#pragma once

#include "core/actor/includes/actor_system.h"
#include "core/bootstrap/includes/bootstrap_config.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace crynet::bootstrap {

/**
 * @cn
 * 最小引导执行器，负责按配置注册服务并驱动启动阶段的初始消息执行。
 *
 * @en
 * Minimal bootstrap executor responsible for registering services from configuration
 * and driving startup-time initial message execution.
 */
class Bootstrapper final {
public:
    /**
     * @cn
     * 注册指定服务名称对应的处理回调。
     *
     * @en
     * Register the handler callback associated with the specified service name.
     */
    void bind_handler(std::string service_name, crynet::core::MessageHandler handler);

    /**
     * @cn
     * 使用给定配置启动系统并注册所有引导服务。
     *
     * @en
     * Start the system with the supplied configuration and register all bootstrap services.
     */
    [[nodiscard]] bool start(const BootstrapConfig& config);

    /**
     * @cn
     * 驱动系统执行直到空闲或达到给定最大步数。
     *
     * @en
     * Drive the system until it becomes idle or the provided step limit is reached.
     */
    [[nodiscard]] std::size_t dispatch_until_idle(std::size_t max_steps = 64);

    /**
     * @cn
     * 根据服务名称查询已注册句柄。
     *
     * @en
     * Resolve the registered handle by service name.
     */
    [[nodiscard]] std::optional<crynet::core::ServiceHandle> find_service_handle(std::string_view service_name) const;

    /**
     * @cn
     * 返回内部 Actor 系统实例。
     *
     * @en
     * Return the internal actor-system instance.
     */
    [[nodiscard]] crynet::core::ActorSystem& system() noexcept;

    /**
     * @cn
     * 以只读方式返回内部 Actor 系统实例。
     *
     * @en
     * Return the internal actor-system instance as read-only.
     */
    [[nodiscard]] const crynet::core::ActorSystem& system() const noexcept;

private:
    /**
     * @cn
     * 未显式绑定时使用的默认空操作处理回调。
     *
     * @en
     * Default no-op handler used when no explicit binding is provided.
     */
    [[nodiscard]] static crynet::core::MessageHandler default_handler();

    /**
     * @cn
     * 内部 Actor 系统实例。
     *
     * @en
     * Internal actor-system instance.
     */
    crynet::core::ActorSystem m_system;

    /**
     * @cn
     * 服务名称到处理回调的绑定表。
     *
     * @en
     * Binding table from service names to handler callbacks.
     */
    std::unordered_map<std::string, crynet::core::MessageHandler> m_handlers;
};

}  // namespace crynet::bootstrap