#pragma once

#include <string_view>

namespace crynet::base {

/**
 * @cn
 * 构建配置访问器，用于统一暴露项目名称、版本和平台信息。
 *
 * @en
 * Build configuration accessor exposing project name, version and platform metadata.
 */
class BuildConfig final {
public:
    /**
     * @cn
     * 返回项目名称。
     *
     * @en
     * Return the project name.
     */
    [[nodiscard]] static std::string_view project_name() noexcept;

    /**
     * @cn
     * 返回当前构建版本号。
     *
     * @en
     * Return the current build version.
     */
    [[nodiscard]] static std::string_view version() noexcept;

    /**
     * @cn
     * 返回当前目标平台名称。
     *
     * @en
     * Return the current target platform name.
     */
    [[nodiscard]] static std::string_view platform() noexcept;
};

}  // namespace crynet::base
