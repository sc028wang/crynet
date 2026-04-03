#pragma once

#include "core/actor/includes/message.h"

#include <string>
#include <string_view>

namespace crynet::core {

/**
 * @cn
 * 服务上下文生命周期状态。
 *
 * @en
 * Lifecycle states for a service context.
 */
enum class ServiceState : std::uint8_t {
    Created = 0,
    Initialized,
    Running,
    Stopped,
    Released,
};

/**
 * @cn
 * 最小服务上下文抽象，负责管理服务句柄、名称和生命周期状态。
 *
 * @en
 * Minimal service-context abstraction that manages service handle, name and lifecycle state.
 */
class ServiceContext final {
public:
    /**
     * @cn
     * 使用服务句柄和可选服务名构造一个最小服务上下文。
     *
     * @en
     * Construct a minimal service context from a handle and an optional service name.
     */
    explicit ServiceContext(ServiceHandle handle, std::string service_name = {});

    /**
     * @cn
     * 返回当前服务句柄。
     *
     * @en
     * Return the current service handle.
     */
    [[nodiscard]] ServiceHandle handle() const noexcept;

    /**
     * @cn
     * 返回当前服务名称视图。
     *
     * @en
     * Return a view of the current service name.
     */
    [[nodiscard]] std::string_view name() const noexcept;

    /**
     * @cn
     * 返回当前生命周期状态。
     *
     * @en
     * Return the current lifecycle state.
     */
    [[nodiscard]] ServiceState state() const noexcept;

    /**
     * @cn
     * 判断服务是否已进入运行态。
     *
     * @en
     * Check whether the service has entered the running state.
     */
    [[nodiscard]] bool is_running() const noexcept;

    /**
     * @cn
     * 将服务从 `Created` 推进到 `Initialized`。
     *
     * @en
     * Advance the service from `Created` to `Initialized`.
     */
    [[nodiscard]] bool initialize() noexcept;

    /**
     * @cn
     * 将服务从 `Initialized` 推进到 `Running`。
     *
     * @en
     * Advance the service from `Initialized` to `Running`.
     */
    [[nodiscard]] bool start() noexcept;

    /**
     * @cn
     * 停止服务并进入 `Stopped` 状态。
     *
     * @en
     * Stop the service and move it into the `Stopped` state.
     */
    void stop() noexcept;

    /**
     * @cn
     * 释放服务上下文并进入 `Released` 状态。
     *
     * @en
     * Release the service context and move it into the `Released` state.
     */
    void release() noexcept;

private:
    /**
     * @cn
     * 当前服务的唯一句柄。
     *
     * @en
     * Unique handle of the current service.
     */
    ServiceHandle m_handle{0};

    /**
     * @cn
     * 当前服务的人类可读名称。
     *
     * @en
     * Human-readable name of the current service.
     */
    std::string m_service_name;

    /**
     * @cn
     * 当前服务生命周期状态。
     *
     * @en
     * Lifecycle state tracked for the current service.
     */
    ServiceState m_state{ServiceState::Created};
};

/**
 * @cn
 * 返回服务状态的稳定文本名称，方便日志和调试输出。
 *
 * @en
 * Return a stable text name for a service state for logging and debugging.
 */
[[nodiscard]] std::string_view service_state_name(ServiceState state) noexcept;

}  // namespace crynet::core