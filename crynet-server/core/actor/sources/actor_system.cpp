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
    return handle;
}

bool ActorSystem::send(Message message) {
    const auto service = m_services.find(message.target_handle());
    if (service == m_services.end()) {
        return false;
    }

    if (!service->second.queue.enqueue(std::move(message))) {
        return false;
    }

    static_cast<void>(m_scheduler.mark_ready(service->first));
    return true;
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
            ++delivered;
        }
    }

    return delivered;
}

bool ActorSystem::dispatch_once() {
    const auto next_handle = m_scheduler.acquire_next();
    if (!next_handle.has_value()) {
        return false;
    }

    const auto service = m_services.find(next_handle.value());
    if (service == m_services.end()) {
        static_cast<void>(m_scheduler.complete(next_handle.value()));
        return false;
    }

    auto message = service->second.queue.dequeue();
    if (!message.has_value()) {
        static_cast<void>(m_scheduler.complete(next_handle.value()));
        return false;
    }

    service->second.handler(service->second.context, message.value());
    static_cast<void>(m_scheduler.complete(next_handle.value()));

    if (!service->second.queue.empty()) {
        static_cast<void>(m_scheduler.mark_ready(next_handle.value()));
    }

    return true;
}

std::size_t ActorSystem::service_count() const noexcept {
    return m_services.size();
}

std::optional<ServiceHandle> ActorSystem::find_service_handle(std::string_view service_name) const {
    return m_handle_registry.find_handle(service_name);
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

}  // namespace crynet::core