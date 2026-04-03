#include "core/monitor/includes/monitor_runner.h"

namespace crynet::monitor {

MonitorRunner::MonitorRunner(crynet::core::WorkerMonitor& monitor) noexcept : m_monitor(monitor) {}

MonitorCheckResult MonitorRunner::check_once() noexcept {
    const auto snapshot = m_monitor.snapshot();
    MonitorCheckResult result{
        .scanned_workers = snapshot.worker_count,
        .detected_stalls = 0,
    };

    for (std::size_t worker_id = 0; worker_id < snapshot.worker_count; ++worker_id) {
        if (m_monitor.check_stalled(worker_id).has_value()) {
            ++result.detected_stalls;
        }
    }

    return result;
}

std::size_t MonitorRunner::run_checks(std::size_t max_cycles) noexcept {
    std::size_t detected_stalls = 0;
    for (std::size_t cycle = 0; cycle < max_cycles; ++cycle) {
        if (m_monitor.quit_requested()) {
            break;
        }

        detected_stalls += check_once().detected_stalls;
    }

    return detected_stalls;
}

}  // namespace crynet::monitor