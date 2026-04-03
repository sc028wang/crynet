#include "core/scheduler/includes/worker_monitor.h"

#include "core/base/includes/logger.h"

#include <string>

namespace crynet::core {

bool WorkerPlugin::stop_requested(std::size_t worker_id) const noexcept {
    static_cast<void>(worker_id);
    return false;
}

void WorkerPlugin::on_dispatch_begin(std::size_t worker_id, const DispatchTrace& trace) noexcept {
    static_cast<void>(worker_id);
    static_cast<void>(trace);
}

void WorkerPlugin::on_dispatch_end(std::size_t worker_id) noexcept {
    static_cast<void>(worker_id);
}

void WorkerPlugin::on_idle(std::size_t worker_id) noexcept {
    static_cast<void>(worker_id);
}

namespace {

std::string build_stall_alert_message(std::size_t worker_id, const WorkerRoute& route) {
    std::string message;
    message.reserve(96);
    message.append("worker ");
    message.append(std::to_string(worker_id));
    message.append(" may be stalled route=");
    message.append(std::string{message_type_name(route.message_type)});
    message.append(" source=");
    message.append(std::to_string(route.source_handle));
    message.append(" target=");
    message.append(std::to_string(route.target_handle));
    message.append(" session=");
    message.append(std::to_string(route.session_id));
    return message;
}

}  // namespace

WorkerMonitor::WorkerMonitor(std::size_t worker_count) : m_workers(worker_count) {}

void WorkerMonitor::mark_sleeping(std::size_t worker_id) noexcept {
    if (!contains_worker(worker_id) || m_workers[worker_id].sleeping) {
        return;
    }

    m_workers[worker_id].sleeping = true;
    ++m_sleeping_workers;
}

void WorkerMonitor::mark_awake(std::size_t worker_id) noexcept {
    if (!contains_worker(worker_id) || !m_workers[worker_id].sleeping) {
        return;
    }

    m_workers[worker_id].sleeping = false;
    --m_sleeping_workers;
    ++m_wake_events;
}

void WorkerMonitor::on_busy_turn(std::size_t worker_id) noexcept {
    if (!contains_worker(worker_id)) {
        return;
    }

    mark_awake(worker_id);
    ++m_busy_turns;
}

void WorkerMonitor::request_quit() noexcept {
    m_quit_requested = true;
}

bool WorkerMonitor::quit_requested() const noexcept {
    return m_quit_requested;
}

bool WorkerMonitor::should_wake(std::size_t busy_workers) const noexcept {
    const auto worker_count = m_workers.size();
    const auto clamped_busy_workers = busy_workers > worker_count ? worker_count : busy_workers;
    return m_sleeping_workers >= worker_count - clamped_busy_workers;
}

WorkerMonitorSnapshot WorkerMonitor::snapshot() const noexcept {
    return WorkerMonitorSnapshot{
        .worker_count = m_workers.size(),
        .sleeping_workers = m_sleeping_workers,
        .wake_events = m_wake_events,
        .busy_turns = m_busy_turns,
        .stall_events = m_stall_events,
        .quit_requested = m_quit_requested,
    };
}

bool WorkerMonitor::stop_requested(std::size_t worker_id) const noexcept {
    static_cast<void>(worker_id);
    return quit_requested();
}

void WorkerMonitor::on_dispatch_begin(std::size_t worker_id, const DispatchTrace& trace) noexcept {
    trigger(worker_id, trace);
}

void WorkerMonitor::on_dispatch_end(std::size_t worker_id) noexcept {
    mark_awake(worker_id);
    clear(worker_id);
    on_busy_turn(worker_id);
}

void WorkerMonitor::on_idle(std::size_t worker_id) noexcept {
    mark_sleeping(worker_id);
}

void WorkerMonitor::set_alert_sink(WorkerMonitorAlertSink sink) {
    m_alert_sink = std::move(sink);
}

void WorkerMonitor::reset_alert_sink() noexcept {
    m_alert_sink = {};
}

void WorkerMonitor::set_logger_alert_enabled(bool enabled) noexcept {
    m_logger_alert_enabled = enabled;
}

void WorkerMonitor::trigger(std::size_t worker_id, const DispatchTrace& trace) noexcept {
    if (!contains_worker(worker_id)) {
        return;
    }

    auto& worker = m_workers[worker_id];
    worker.route = WorkerRoute{
        .message_type = trace.message_type,
        .source_handle = trace.source_handle,
        .target_handle = trace.target_handle,
        .session_id = trace.session_id,
    };
    ++worker.version;
}

void WorkerMonitor::clear(std::size_t worker_id) noexcept {
    if (!contains_worker(worker_id)) {
        return;
    }

    auto& worker = m_workers[worker_id];
    worker.route = WorkerRoute{};
    ++worker.version;
    worker.checked_version = worker.version;
}

std::optional<WorkerRoute> WorkerMonitor::active_route(std::size_t worker_id) const noexcept {
    if (!contains_worker(worker_id) || !m_workers[worker_id].route.active()) {
        return std::nullopt;
    }

    return m_workers[worker_id].route;
}

std::optional<WorkerRoute> WorkerMonitor::check_stalled(std::size_t worker_id) noexcept {
    if (!contains_worker(worker_id)) {
        return std::nullopt;
    }

    auto& worker = m_workers[worker_id];
    if (!worker.route.active()) {
        worker.checked_version = worker.version;
        return std::nullopt;
    }

    if (worker.version == worker.checked_version) {
        ++m_stall_events;
        emit_stall_alert(worker_id, worker.route);
        return worker.route;
    }

    worker.checked_version = worker.version;
    return std::nullopt;
}

bool WorkerMonitor::contains_worker(std::size_t worker_id) const noexcept {
    return worker_id < m_workers.size();
}

void WorkerMonitor::emit_stall_alert(std::size_t worker_id, const WorkerRoute& route) {
    const WorkerMonitorAlert alert{
        .worker_id = worker_id,
        .route = route,
        .message = build_stall_alert_message(worker_id, route),
    };

    if (m_logger_alert_enabled) {
        Logger::default_logger().log(LogLevel::Error, "monitor", alert.message);
    }

    if (m_alert_sink) {
        m_alert_sink(alert);
    }
}

}  // namespace crynet::core