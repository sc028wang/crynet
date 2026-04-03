#include "core/bootstrap/includes/bootstrapper.h"

namespace crynet::bootstrap {

void Bootstrapper::bind_handler(std::string service_name, crynet::core::MessageHandler handler) {
    m_handlers.insert_or_assign(std::move(service_name), std::move(handler));
}

bool Bootstrapper::start(const BootstrapConfig& config) {
    for (const auto& service : config.services()) {
        auto handler = default_handler();
        if (const auto bound = m_handlers.find(service.name); bound != m_handlers.end()) {
            handler = bound->second;
        }

        const auto handle = m_system.register_service(service.name, std::move(handler));
        if (!handle.has_value()) {
            return false;
        }

        if (!service.boot_payload.empty() &&
            !m_system.send(crynet::core::Message::from_text(
                crynet::core::MessageType::System,
                0,
                handle.value(),
                0,
                service.boot_payload
            ))) {
            return false;
        }
    }

    return true;
}

std::size_t Bootstrapper::dispatch_until_idle(std::size_t max_steps) {
    std::size_t executed = 0;
    while (executed < max_steps && m_system.dispatch_once()) {
        ++executed;
    }

    return executed;
}

std::optional<crynet::core::ServiceHandle> Bootstrapper::find_service_handle(std::string_view service_name) const {
    return m_system.find_service_handle(service_name);
}

crynet::core::ActorSystem& Bootstrapper::system() noexcept {
    return m_system;
}

const crynet::core::ActorSystem& Bootstrapper::system() const noexcept {
    return m_system;
}

crynet::core::MessageHandler Bootstrapper::default_handler() {
    return [](crynet::core::ServiceContext&, const crynet::core::Message&) {
        // The default bootstrap handler intentionally performs no work.
    };
}

}  // namespace crynet::bootstrap