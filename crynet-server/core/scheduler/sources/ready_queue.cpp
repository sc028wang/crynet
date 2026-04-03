#include "core/scheduler/includes/ready_queue.h"

namespace crynet::core {

bool ReadyQueue::push(ServiceHandle handle) {
    if (handle == 0 || contains(handle)) {
        return false;
    }

    m_handles.push_back(handle);
    m_queued_handles.insert(handle);
    return true;
}

std::optional<ServiceHandle> ReadyQueue::pop() noexcept {
    if (m_handles.empty()) {
        return std::nullopt;
    }

    const auto handle = m_handles.front();
    m_handles.pop_front();
    m_queued_handles.erase(handle);
    return handle;
}

bool ReadyQueue::contains(ServiceHandle handle) const noexcept {
    return m_queued_handles.find(handle) != m_queued_handles.end();
}

bool ReadyQueue::empty() const noexcept {
    return m_handles.empty();
}

std::size_t ReadyQueue::size() const noexcept {
    return m_handles.size();
}

}  // namespace crynet::core