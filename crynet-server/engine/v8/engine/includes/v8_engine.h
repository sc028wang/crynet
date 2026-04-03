#pragma once

#include <string_view>

namespace crynet::engine::v8 {

/**
 * @cn
 * `V8` 引擎占位封装，用于在 T0 阶段确认三方依赖接入链路可用。
 *
 * @en
 * Placeholder `V8` engine wrapper used during T0 to validate the third-party integration path.
 */
class V8Engine final {
public:
    /**
     * @cn
     * 判断当前工程是否已经接通 `V8` 头文件依赖。
     *
     * @en
     * Check whether the project has a working `V8` header integration path.
     */
    [[nodiscard]] static bool vendor_ready() noexcept;

    /**
     * @cn
     * 返回脚本引擎名称。
     *
     * @en
     * Return the scripting engine name.
     */
    [[nodiscard]] static std::string_view engine_name() noexcept;
};

}  // namespace crynet::engine::v8
