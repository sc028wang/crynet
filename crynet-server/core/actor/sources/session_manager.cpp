#include "core/actor/includes/session_manager.h"

namespace crynet::core {

SessionId SessionManager::begin(ServiceHandle caller_handle, ServiceHandle callee_handle) noexcept {
    while (m_next_session_id == 0 || contains(m_next_session_id)) {
        ++m_next_session_id;
    }

    const auto session_id = m_next_session_id++;
    m_entries.emplace(session_id, Entry{
        .caller_handle = caller_handle,
        .callee_handle = callee_handle,
    });
    return session_id;
}

std::optional<ServiceHandle> SessionManager::lookup_requester(
    SessionId session_id,
    ServiceHandle responder_handle
) const noexcept {
    const auto entry = m_entries.find(session_id);
    if (entry == m_entries.end() || entry->second.callee_handle != responder_handle) {
        return std::nullopt;
    }

    return entry->second.caller_handle;
}

bool SessionManager::complete(SessionId session_id, ServiceHandle responder_handle) noexcept {
    const auto entry = m_entries.find(session_id);
    if (entry == m_entries.end() || entry->second.callee_handle != responder_handle) {
        return false;
    }

    m_entries.erase(entry);
    return true;
}

bool SessionManager::contains(SessionId session_id) const noexcept {
    return m_entries.find(session_id) != m_entries.end();
}

std::size_t SessionManager::size() const noexcept {
    return m_entries.size();
}

}  // namespace crynet::core