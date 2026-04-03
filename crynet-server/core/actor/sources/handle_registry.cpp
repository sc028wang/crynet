#include "core/actor/includes/handle_registry.h"

namespace crynet::core {

ServiceHandle HandleRegistry::allocate() noexcept {
    // Keep zero reserved as an invalid handle sentinel.
    while (m_next_handle == 0 || contains(m_next_handle)) {
        ++m_next_handle;
    }

    return m_next_handle++;
}

bool HandleRegistry::register_service(const ServiceContext& context) {
    if (context.handle() == 0 || contains(context.handle())) {
        return false;
    }

    if (!context.name().empty()) {
        if (m_name_to_handle.find(std::string{context.name()}) != m_name_to_handle.end()) {
            return false;
        }

        m_name_to_handle.emplace(std::string{context.name()}, context.handle());
    }

    m_handle_to_name.emplace(context.handle(), std::string{context.name()});
    return true;
}

bool HandleRegistry::unregister(ServiceHandle handle) noexcept {
    const auto entry = m_handle_to_name.find(handle);
    if (entry == m_handle_to_name.end()) {
        return false;
    }

    if (!entry->second.empty()) {
        m_name_to_handle.erase(entry->second);
    }

    m_handle_to_name.erase(entry);
    return true;
}

bool HandleRegistry::contains(ServiceHandle handle) const noexcept {
    return m_handle_to_name.find(handle) != m_handle_to_name.end();
}

std::optional<ServiceHandle> HandleRegistry::find_handle(std::string_view service_name) const {
    const auto entry = m_name_to_handle.find(std::string{service_name});
    if (entry == m_name_to_handle.end()) {
        return std::nullopt;
    }

    return entry->second;
}

std::optional<std::string> HandleRegistry::find_name(ServiceHandle handle) const {
    const auto entry = m_handle_to_name.find(handle);
    if (entry == m_handle_to_name.end()) {
        return std::nullopt;
    }

    return entry->second;
}

std::size_t HandleRegistry::size() const noexcept {
    return m_handle_to_name.size();
}

}  // namespace crynet::core