#include "core/actor/includes/message_queue.h"

namespace crynet::core {

MessageQueue::MessageQueue(ServiceHandle owner_handle) noexcept
    : m_owner_handle(owner_handle) {}

ServiceHandle MessageQueue::owner_handle() const noexcept {
    return m_owner_handle;
}

bool MessageQueue::enqueue(Message message) {
    // A per-service queue only accepts messages routed to its owner.
    if (message.target_handle() != m_owner_handle) {
        return false;
    }

    m_messages.push_back(std::move(message));
    const auto current_size = m_messages.size();
    if (current_size > m_peak_size) {
        m_peak_size = current_size;
    }

    while (current_size > m_overload_threshold) {
        m_overload_size = current_size;
        m_overload_threshold *= 2;
    }

    return true;
}

std::optional<Message> MessageQueue::dequeue() noexcept {
    if (m_messages.empty()) {
        return std::nullopt;
    }

    Message message = std::move(m_messages.front());
    m_messages.pop_front();
    if (m_messages.empty()) {
        m_overload_threshold = 1024;
    }
    return message;
}

bool MessageQueue::empty() const noexcept {
    return m_messages.empty();
}

std::size_t MessageQueue::size() const noexcept {
    return m_messages.size();
}

std::optional<std::size_t> MessageQueue::take_overload() noexcept {
    if (m_overload_size == 0) {
        return std::nullopt;
    }

    const auto overload_size = m_overload_size;
    m_overload_size = 0;
    return overload_size;
}

std::size_t MessageQueue::peak_size() const noexcept {
    return m_peak_size;
}

}  // namespace crynet::core