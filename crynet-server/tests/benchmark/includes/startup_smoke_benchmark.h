#pragma once

namespace crynet::tests::benchmark {

/**
 * @cn
 * 启动链路烟雾压测入口，用于验证 benchmark 基础设施已经接通。
 *
 * @en
 * Startup-path smoke benchmark entry used to validate that the benchmark pipeline is wired.
 */
class StartupSmokeBenchmark final {
public:
    /**
     * @cn
     * 执行基准测试并返回退出码。
     *
     * @en
     * Execute the benchmark and return the process exit code.
     */
    [[nodiscard]] static int run() noexcept;
};

}  // namespace crynet::tests::benchmark
