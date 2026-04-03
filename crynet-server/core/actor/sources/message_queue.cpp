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
    return true;
}

std::optional<Message> MessageQueue::dequeue() noexcept {
    if (m_messages.empty()) {
        return std::nullopt;
    }

    Message message = std::move(m_messages.front());
    m_messages.pop_front();
    return message;
}

bool MessageQueue::empty() const noexcept {
    return m_messages.empty();
}

std::size_t MessageQueue::size() const noexcept {
    return m_messages.size();
}

}  // namespace crynet::core