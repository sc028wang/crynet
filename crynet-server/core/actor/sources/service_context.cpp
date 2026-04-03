#include "core/actor/includes/service_context.h"

namespace crynet::core {

ServiceContext::ServiceContext(ServiceHandle handle, std::string service_name)
    : m_handle(handle),
      m_service_name(std::move(service_name)) {}

ServiceHandle ServiceContext::handle() const noexcept {
    return m_handle;
}

std::string_view ServiceContext::name() const noexcept {
    return m_service_name;
}

ServiceState ServiceContext::state() const noexcept {
    return m_state;
}

bool ServiceContext::is_running() const noexcept {
    return m_state == ServiceState::Running;
}

bool ServiceContext::initialize() noexcept {
    // Initialization is a one-way transition and only valid directly from Created.
    if (m_state != ServiceState::Created) {
        return false;
    }

    m_state = ServiceState::Initialized;
    return true;
}

bool ServiceContext::start() noexcept {
    // Starting is only valid after initialization has completed.
    if (m_state != ServiceState::Initialized) {
        return false;
    }

    m_state = ServiceState::Running;
    return true;
}

void ServiceContext::stop() noexcept {
    // Stopping is tolerant for both initialized and running services to simplify bootstrap teardown.
    if (m_state == ServiceState::Running || m_state == ServiceState::Initialized) {
        m_state = ServiceState::Stopped;
    }
}

void ServiceContext::release() noexcept {
    // Release is terminal in the current T1 model.
    m_state = ServiceState::Released;
}

std::string_view service_state_name(ServiceState state) noexcept {
    // Keep state strings stable so diagnostics and tests do not drift.
    switch (state) {
        case ServiceState::Created:
            return "created";
        case ServiceState::Initialized:
            return "initialized";
        case ServiceState::Running:
            return "running";
        case ServiceState::Stopped:
            return "stopped";
        case ServiceState::Released:
            return "released";
    }

    return "unknown";
}

}  // namespace crynet::core