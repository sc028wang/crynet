#include "core/scheduler/includes/worker_scheduler.h"

namespace crynet::core {

bool WorkerScheduler::mark_ready(ServiceHandle handle) {
    if (handle == 0 || is_in_flight(handle)) {
        return false;
    }

    return m_ready_queue.push(handle);
}

std::optional<ServiceHandle> WorkerScheduler::acquire_next() noexcept {
    const auto next_handle = m_ready_queue.pop();
    if (!next_handle.has_value()) {
        return std::nullopt;
    }

    m_in_flight_handles.insert(next_handle.value());
    return next_handle;
}

bool WorkerScheduler::complete(ServiceHandle handle) noexcept {
    return m_in_flight_handles.erase(handle) > 0;
}

bool WorkerScheduler::is_in_flight(ServiceHandle handle) const noexcept {
    return m_in_flight_handles.find(handle) != m_in_flight_handles.end();
}

std::size_t WorkerScheduler::pending_count() const noexcept {
    return m_ready_queue.size();
}

std::size_t WorkerScheduler::in_flight_count() const noexcept {
    return m_in_flight_handles.size();
}

}  // namespace crynet::core