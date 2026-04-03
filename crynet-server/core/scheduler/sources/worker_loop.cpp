#include "core/scheduler/includes/worker_loop.h"

namespace crynet::core {

WorkerLoop::WorkerLoop(ActorSystem& system) noexcept : m_system(system) {}

WorkerLoopTurn WorkerLoop::run_once(
    std::chrono::steady_clock::time_point now,
    std::size_t max_messages_per_service,
    DispatchObserver observer
) {
    if (m_stop_requested) {
        return {};
    }

    WorkerLoopTurn turn;
    turn.injected_timer_messages = m_system.pump_timers(now);
    turn.dispatched_messages = m_system.dispatch_batch(max_messages_per_service, std::move(observer));
    return turn;
}

std::size_t WorkerLoop::run_until_idle(
    std::size_t max_cycles,
    std::size_t max_messages_per_service
) {
    std::size_t total_dispatched = 0;
    for (std::size_t cycle = 0; cycle < max_cycles; ++cycle) {
        if (m_stop_requested) {
            break;
        }

        const auto turn = run_once(std::chrono::steady_clock::now(), max_messages_per_service);
        if (turn.idle()) {
            break;
        }

        total_dispatched += turn.dispatched_messages;
    }

    return total_dispatched;
}

void WorkerLoop::request_stop() noexcept {
    m_stop_requested = true;
}

void WorkerLoop::reset_stop() noexcept {
    m_stop_requested = false;
}

bool WorkerLoop::stop_requested() const noexcept {
    return m_stop_requested;
}

}  // namespace crynet::core