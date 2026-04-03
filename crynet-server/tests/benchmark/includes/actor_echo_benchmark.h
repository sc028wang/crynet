#pragma once

namespace crynet::tests::benchmark {

/**
 * @cn
 * Actor echo 压测入口，用于验证 T1 阶段的 request/response 链路在高消息量下稳定运行。
 *
 * @en
 * Actor echo benchmark entry used to validate that the T1 request/response path runs stably under high message volume.
 */
class ActorEchoBenchmark final {
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