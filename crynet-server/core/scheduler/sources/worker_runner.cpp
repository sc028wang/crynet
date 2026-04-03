#include "core/scheduler/includes/worker_runner.h"

namespace crynet::core {

WorkerRunner::WorkerRunner(ActorSystem& system, std::size_t worker_id) noexcept
    : m_loop(system), m_worker_id(worker_id) {}

WorkerRunner::WorkerRunner(ActorSystem& system, WorkerMonitor& monitor, std::size_t worker_id) noexcept
    : WorkerRunner(system, worker_id) {
    add_plugin(monitor);
}

void WorkerRunner::add_plugin(WorkerPlugin& plugin) noexcept {
    m_plugins.push_back(&plugin);
}

std::size_t WorkerRunner::plugin_count() const noexcept {
    return m_plugins.size();
}

WorkerLoopTurn WorkerRunner::run_once(
    std::chrono::steady_clock::time_point now,
    std::size_t max_messages_per_service
) {
    if (!m_plugins.empty()) {
        for (const auto* plugin : m_plugins) {
            if (plugin->stop_requested(m_worker_id)) {
                m_loop.request_stop();
                return {};
            }
        }
    }

    const auto turn = m_plugins.empty()
        ? m_loop.run_once(now, max_messages_per_service)
        : m_loop.run_once(
              now,
              max_messages_per_service,
              [this](const DispatchTrace& trace) {
                  for (auto* plugin : m_plugins) {
                      plugin->on_dispatch_begin(m_worker_id, trace);
                  }
              }
          );
    if (turn.idle()) {
        for (auto* plugin : m_plugins) {
            plugin->on_idle(m_worker_id);
        }
        return turn;
    }

    for (auto* plugin : m_plugins) {
        plugin->on_dispatch_end(m_worker_id);
    }
    return turn;
}

std::size_t WorkerRunner::run_until_idle(
    std::size_t max_cycles,
    std::size_t max_messages_per_service
) {
    std::size_t total_dispatched = 0;
    for (std::size_t cycle = 0; cycle < max_cycles; ++cycle) {
        const auto turn = run_once(std::chrono::steady_clock::now(), max_messages_per_service);
        if (turn.idle()) {
            break;
        }

        total_dispatched += turn.dispatched_messages;
    }

    return total_dispatched;
}

}  // namespace crynet::core