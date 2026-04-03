#include "core/actor/includes/actor_system.h"

#include <utility>

namespace crynet::core {

std::optional<ServiceHandle> ActorSystem::register_service(std::string service_name, MessageHandler handler) {
    const auto handle = m_handle_registry.allocate();
    ServiceContext context{handle, std::move(service_name)};
    if (!context.initialize() || !context.start()) {
        return std::nullopt;
    }

    if (!m_handle_registry.register_service(context)) {
        return std::nullopt;
    }

    m_services.emplace(handle, ServiceSlot{
        .context = std::move(context),
        .queue = MessageQueue{handle},
        .handler = std::move(handler),
    });
    m_metrics.on_service_registered();
    return handle;
}

bool ActorSystem::send(Message message) {
    const auto service = m_services.find(message.target_handle());
    if (service == m_services.end()) {
        m_metrics.on_message_dropped();
        return false;
    }

    if (!service->second.context.is_running()) {
        m_metrics.on_message_dropped();
        return false;
    }

    if (!service->second.queue.enqueue(std::move(message))) {
        m_metrics.on_message_dropped();
        return false;
    }

    m_metrics.on_message_enqueued();
    m_metrics.observe_queue_depth(service->second.queue.size());
    if (service->second.queue.take_overload().has_value()) {
        m_metrics.on_queue_overload();
    }
    static_cast<void>(m_scheduler.mark_ready(service->first));
    return true;
}

bool ActorSystem::stop_service(ServiceHandle handle) noexcept {
    const auto service = m_services.find(handle);
    if (service == m_services.end()) {
        return false;
    }

    service->second.context.stop();
    return service->second.context.state() == ServiceState::Stopped;
}

bool ActorSystem::release_service(ServiceHandle handle) noexcept {
    const auto service = m_services.find(handle);
    if (service == m_services.end()) {
        return false;
    }

    service->second.context.stop();
    service->second.context.release();
    static_cast<void>(m_session_manager.cancel_for_handle(handle));
    static_cast<void>(m_timer_queue.cancel_for_target(handle));
    static_cast<void>(m_handle_registry.unregister(handle));
    m_services.erase(service);
    return true;
}

std::size_t ActorSystem::shutdown() noexcept {
    std::vector<ServiceHandle> handles;
    handles.reserve(m_services.size());
    for (const auto& [handle, service] : m_services) {
        static_cast<void>(service);
        handles.push_back(handle);
    }

    std::size_t released = 0;
    for (const auto handle : handles) {
        if (release_service(handle)) {
            ++released;
        }
    }

    return released;
}

std::optional<SessionId> ActorSystem::call(
    ServiceHandle source_handle,
    ServiceHandle target_handle,
    std::string payload_text
) {
    if (m_services.find(source_handle) == m_services.end() || m_services.find(target_handle) == m_services.end()) {
        return std::nullopt;
    }

    const auto session_id = m_session_manager.begin(source_handle, target_handle);
    if (!send(Message::from_text(
            MessageType::Request,
            source_handle,
            target_handle,
            session_id,
            payload_text
        ))) {
        return std::nullopt;
    }

    return session_id;
}

bool ActorSystem::respond(
    ServiceHandle source_handle,
    SessionId session_id,
    std::string payload_text
) {
    const auto requester_handle = m_session_manager.lookup_requester(session_id, source_handle);
    if (!requester_handle.has_value()) {
        return false;
    }

    if (!send(Message::from_text(
            MessageType::Response,
            source_handle,
            requester_handle.value(),
            session_id,
            payload_text
        ))) {
        return false;
    }

    return m_session_manager.complete(session_id, source_handle);
}

TimerId ActorSystem::schedule_timeout(
    ServiceHandle target_handle,
    SessionId session_id,
    std::chrono::milliseconds delay,
    std::string payload_text
) {
    return m_timer_queue.schedule_after(target_handle, session_id, delay, std::move(payload_text));
}

bool ActorSystem::cancel_timeout(TimerId timer_id) noexcept {
    return m_timer_queue.cancel(timer_id);
}

std::size_t ActorSystem::pump_timers(std::chrono::steady_clock::time_point now) {
    std::size_t delivered = 0;
    for (auto& message : m_timer_queue.poll_expired(now)) {
        if (send(std::move(message))) {
            m_metrics.on_timer_message();
            ++delivered;
        }
    }

    return delivered;
}

std::size_t ActorSystem::dispatch_batch(
    std::size_t max_messages_per_service,
    DispatchObserver observer
) {
    if (max_messages_per_service == 0) {
        max_messages_per_service = 1;
    }

    const auto next_handle = m_scheduler.acquire_next();
    if (!next_handle.has_value()) {
        return 0;
    }

    const auto service = m_services.find(next_handle.value());
    if (service == m_services.end()) {
        static_cast<void>(m_scheduler.complete(next_handle.value()));
        return 0;
    }

    std::size_t dispatched = 0;
    while (dispatched < max_messages_per_service) {
        auto message = service->second.queue.dequeue();
        if (!message.has_value()) {
            break;
        }

        if (observer) {
            observer(DispatchTrace{
                .message_type = message->type(),
                .source_handle = message->source_handle(),
                .target_handle = message->target_handle(),
                .session_id = message->session_id(),
            });
        }

        service->second.handler(service->second.context, message.value());
        m_metrics.on_message_dispatched();
        ++dispatched;
    }

    static_cast<void>(m_scheduler.complete(next_handle.value()));

    if (!service->second.queue.empty() && service->second.context.is_running()) {
        static_cast<void>(m_scheduler.mark_ready(next_handle.value()));
    }

    return dispatched;
}

std::size_t ActorSystem::run_until_idle(
    std::size_t max_batches,
    std::size_t max_messages_per_service
) {
    std::size_t total_dispatched = 0;
    std::size_t executed_batches = 0;
    while (executed_batches < max_batches) {
        const auto dispatched = dispatch_batch(max_messages_per_service);
        if (dispatched == 0) {
            break;
        }

        total_dispatched += dispatched;
        ++executed_batches;
    }

    return total_dispatched;
}

bool ActorSystem::dispatch_once() {
    return dispatch_batch(1) > 0;
}

std::size_t ActorSystem::service_count() const noexcept {
    return m_services.size();
}

std::optional<ServiceHandle> ActorSystem::find_service_handle(std::string_view service_name) const {
    return m_handle_registry.find_handle(service_name);
}

std::optional<ServiceState> ActorSystem::service_state(ServiceHandle handle) const noexcept {
    const auto service = m_services.find(handle);
    if (service == m_services.end()) {
        return std::nullopt;
    }

    return service->second.context.state();
}

std::size_t ActorSystem::pending_sessions() const noexcept {
    return m_session_manager.size();
}

std::size_t ActorSystem::pending_messages(ServiceHandle handle) const noexcept {
    const auto service = m_services.find(handle);
    if (service == m_services.end()) {
        return 0;
    }

    return service->second.queue.size();
}

MetricsSystemSnapshot ActorSystem::metrics() const noexcept {
    auto snapshot = m_metrics.snapshot();
    snapshot.registered_services = m_services.size();
    snapshot.pending_sessions = m_session_manager.size();
    snapshot.ready_services = m_scheduler.pending_count();
    snapshot.in_flight_services = m_scheduler.in_flight_count();

    std::size_t queued_messages = 0;
    for (const auto& [handle, service] : m_services) {
        static_cast<void>(handle);
        queued_messages += service.queue.size();
    }
    snapshot.queued_messages = queued_messages;

    return snapshot;
}

}  // namespace crynet::core