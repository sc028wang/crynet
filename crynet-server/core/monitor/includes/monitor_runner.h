#pragma once

#include "core/scheduler/includes/worker_monitor.h"

#include <cstddef>

namespace crynet::monitor {

/**
 * @cn
 * monitor 单轮检查结果，汇总本次扫描发现的 stall 数量。
 *
 * @en
 * Result of one monitor check pass, aggregating how many stalls were found during the scan.
 */
struct MonitorCheckResult final {
    /**
     * @cn
     * 本轮扫描到的 worker 总数。
     *
     * @en
     * Total number of workers scanned during the current pass.
     */
    std::size_t scanned_workers{0};

    /**
     * @cn
     * 本轮检测到的 stall 数量。
     *
     * @en
     * Number of stalls detected during the current pass.
     */
    std::size_t detected_stalls{0};
};

/**
 * @cn
 * monitor 运行器，负责按周期扫描 worker monitor 并触发 stall 检查。
 *
 * @en
 * Monitor runner responsible for periodically scanning the worker monitor and triggering stall checks.
 */
class MonitorRunner final {
public:
    /**
     * @cn
     * 创建一个绑定到指定 worker monitor 的 monitor 运行器。
     *
     * @en
     * Create a monitor runner bound to the specified worker monitor.
     */
    explicit MonitorRunner(crynet::core::WorkerMonitor& monitor) noexcept;

    /**
     * @cn
     * 执行一轮 monitor 扫描，并返回本轮检查结果。
     *
     * @en
     * Execute one monitor scan and return the result of the current pass.
     */
    [[nodiscard]] MonitorCheckResult check_once() noexcept;

    /**
     * @cn
     * 连续执行多轮 monitor 扫描，直到达到上限或 monitor 收到退出请求，返回累计 stall 数量。
     *
     * @en
     * Execute multiple monitor scans until the limit is reached or the monitor receives a quit request,
     * and return the cumulative number of detected stalls.
     */
    [[nodiscard]] std::size_t run_checks(std::size_t max_cycles = 5) noexcept;

private:
    /**
     * @cn
     * 被当前运行器扫描的 worker monitor。
     *
     * @en
     * Worker monitor scanned by the current runner.
     */
    crynet::core::WorkerMonitor& m_monitor;
};

}  // namespace crynet::monitor