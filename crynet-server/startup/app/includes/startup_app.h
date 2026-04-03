#pragma once

#include <string>

namespace crynet::startup {

/**
 * @cn
 * T0 阶段的启动壳体，负责初始化、输出启动横幅以及执行最小可运行流程。
 *
 * @en
 * T0 startup shell responsible for initialization, startup banner output, and the minimum runnable flow.
 */
class StartupApp final {
public:
    /**
     * @cn
     * 初始化启动壳体。
     *
     * @en
     * Initialize the startup shell.
     */
    [[nodiscard]] bool initialize() noexcept;

    /**
     * @cn
     * 关闭启动壳体并重置内部状态。
     *
     * @en
     * Shut down the startup shell and reset its internal state.
     */
    void shutdown() noexcept;

    /**
     * @cn
     * 判断启动壳体是否已完成初始化。
     *
     * @en
     * Check whether the startup shell has already been initialized.
     */
    [[nodiscard]] bool is_initialized() const noexcept;

    /**
     * @cn
     * 生成启动横幅文本，方便输出当前构建信息。
     *
     * @en
     * Build a startup banner string with the current build metadata.
     */
    [[nodiscard]] std::string banner() const;

    /**
     * @cn
     * 执行一次启动流程并返回进程退出码。
     *
     * @en
     * Run the startup flow once and return the process exit code.
     */
    [[nodiscard]] int run() noexcept;

private:
    bool m_b_initialized{false};
};

}  // namespace crynet::startup
