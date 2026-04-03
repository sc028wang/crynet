#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace crynet::bootstrap {

/**
 * @cn
 * 单个引导服务条目，描述要注册的服务名称和初始投递文本。
 *
 * @en
 * Single bootstrap-service entry describing the service name to register and the initial text payload to deliver.
 */
struct BootstrapServiceConfig final {
    /**
     * @cn
     * 服务逻辑名称。
     *
     * @en
     * Logical service name.
     */
    std::string name;

    /**
     * @cn
     * 引导阶段投递给该服务的初始文本载荷。
     *
     * @en
     * Initial text payload delivered to the service during bootstrap.
     */
    std::string boot_payload;
};

/**
 * @cn
 * 最小引导配置对象，负责从文本中加载服务列表和超时参数。
 *
 * @en
 * Minimal bootstrap configuration object responsible for loading service lists and timeout settings from text.
 */
class BootstrapConfig final {
public:
    /**
     * @cn
     * 从文本配置中解析引导配置；格式支持 `service <name> <payload>` 和 `timeout_ms <n>`。
     *
     * @en
     * Parse bootstrap configuration from text; supported directives are `service <name> <payload>` and `timeout_ms <n>`.
     */
    [[nodiscard]] static std::optional<BootstrapConfig> from_text(std::string_view config_text);

    /**
     * @cn
     * 返回已配置的引导服务条目列表。
     *
     * @en
     * Return the configured bootstrap-service entries.
     */
    [[nodiscard]] const std::vector<BootstrapServiceConfig>& services() const noexcept;

    /**
     * @cn
     * 返回引导阶段使用的默认超时毫秒数。
     *
     * @en
     * Return the default timeout duration in milliseconds used during bootstrap.
     */
    [[nodiscard]] std::chrono::milliseconds timeout() const noexcept;

private:
    /**
     * @cn
     * 已解析的引导服务条目列表。
     *
     * @en
     * Parsed bootstrap-service entry list.
     */
    std::vector<BootstrapServiceConfig> m_services;

    /**
     * @cn
     * 引导默认超时时间。
     *
     * @en
     * Default bootstrap timeout duration.
     */
    std::chrono::milliseconds m_timeout{0};
};

}  // namespace crynet::bootstrap