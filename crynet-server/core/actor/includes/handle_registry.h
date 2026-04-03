#pragma once

#include "core/actor/includes/service_context.h"

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace crynet::core {

/**
 * @cn
 * 最小服务句柄注册表，负责分配服务句柄并维护句柄到名称的双向映射。
 *
 * @en
 * Minimal service-handle registry responsible for allocating handles and maintaining
 * bidirectional mappings between handles and service names.
 */
class HandleRegistry final {
public:
    /**
     * @cn
     * 分配一个新的服务句柄，保证不会返回零句柄。
     *
     * @en
     * Allocate a new service handle and guarantee that zero is never returned.
     */
    [[nodiscard]] ServiceHandle allocate() noexcept;

    /**
     * @cn
     * 注册服务上下文中的句柄与名称信息。
     *
     * @en
     * Register the handle and name metadata carried by a service context.
     */
    [[nodiscard]] bool register_service(const ServiceContext& context);

    /**
     * @cn
     * 根据句柄移除已注册的服务信息。
     *
     * @en
     * Remove a registered service entry by handle.
     */
    [[nodiscard]] bool unregister(ServiceHandle handle) noexcept;

    /**
     * @cn
     * 判断指定句柄当前是否已注册。
     *
     * @en
     * Check whether the specified handle is currently registered.
     */
    [[nodiscard]] bool contains(ServiceHandle handle) const noexcept;

    /**
     * @cn
     * 根据服务名称解析已注册句柄。
     *
     * @en
     * Resolve a registered handle from a service name.
     */
    [[nodiscard]] std::optional<ServiceHandle> find_handle(std::string_view service_name) const;

    /**
     * @cn
     * 根据句柄查询已注册的服务名称。
     *
     * @en
     * Query the registered service name by handle.
     */
    [[nodiscard]] std::optional<std::string> find_name(ServiceHandle handle) const;

    /**
     * @cn
     * 返回当前已注册服务数量。
     *
     * @en
     * Return the number of currently registered services.
     */
    [[nodiscard]] std::size_t size() const noexcept;

private:
    /**
     * @cn
     * 下一个待分配的服务句柄序号。
     *
     * @en
     * Next service-handle sequence value to allocate.
     */
    ServiceHandle m_next_handle{1};

    /**
     * @cn
     * 句柄到服务名称的映射表。
     *
     * @en
     * Mapping table from service handles to service names.
     */
    std::unordered_map<ServiceHandle, std::string> m_handle_to_name;

    /**
     * @cn
     * 服务名称到句柄的反向映射表。
     *
     * @en
     * Reverse mapping table from service names to service handles.
     */
    std::unordered_map<std::string, ServiceHandle> m_name_to_handle;
};

}  // namespace crynet::core