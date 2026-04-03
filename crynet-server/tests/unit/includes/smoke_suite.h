#pragma once

namespace crynet::tests::unit {

/**
 * @cn
 * 基础烟雾测试集合，用于验证运行时骨架是否能正常初始化。
 *
 * @en
 * Basic smoke-test suite validating that the runtime scaffold can initialize correctly.
 */
class SmokeSuite final {
public:
    /**
     * @cn
     * 运行测试并返回退出码。
     *
     * @en
     * Execute the smoke tests and return the process exit code.
     */
    [[nodiscard]] static int run() noexcept;
};

}  // namespace crynet::tests::unit
