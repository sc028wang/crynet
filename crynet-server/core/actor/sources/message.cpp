#include "core/actor/includes/message.h"

namespace crynet::core {

Message::Message(
    MessageType type,
    ServiceHandle source_handle,
    ServiceHandle target_handle,
    SessionId session_id,
    std::vector<std::byte> payload
) noexcept
    : m_type(type),
      m_source_handle(source_handle),
      m_target_handle(target_handle),
      m_session_id(session_id),
      m_payload(std::move(payload)) {}

Message Message::from_text(
    MessageType type,
    ServiceHandle source_handle,
    ServiceHandle target_handle,
    SessionId session_id,
    std::string_view text_payload
) {
    // Convert the UTF-8 input into the runtime's byte-oriented payload storage.
    std::vector<std::byte> payload;
    payload.reserve(text_payload.size());
    for (const char value : text_payload) {
        payload.push_back(static_cast<std::byte>(value));
    }

    return Message{type, source_handle, target_handle, session_id, std::move(payload)};
}

MessageType Message::type() const noexcept {
    return m_type;
}

ServiceHandle Message::source_handle() const noexcept {
    return m_source_handle;
}

ServiceHandle Message::target_handle() const noexcept {
    return m_target_handle;
}

SessionId Message::session_id() const noexcept {
    return m_session_id;
}

bool Message::empty() const noexcept {
    return m_payload.empty();
}

std::size_t Message::payload_size() const noexcept {
    return m_payload.size();
}

const std::vector<std::byte>& Message::payload() const noexcept {
    return m_payload;
}

std::string_view Message::text_payload() const noexcept {
    // T1 keeps payload ownership inside the message, so returning a view is safe here.
    return {
        reinterpret_cast<const char*>(m_payload.data()),
        m_payload.size()
    };
}

std::string_view message_type_name(MessageType type) noexcept {
    // Keep names stable so logs, tests and future protocol traces use the same vocabulary.
    switch (type) {
        case MessageType::Invalid:
            return "invalid";
        case MessageType::Request:
            return "request";
        case MessageType::Response:
            return "response";
        case MessageType::Event:
            return "event";
        case MessageType::Timer:
            return "timer";
        case MessageType::System:
            return "system";
    }

    return "unknown";
}

}  // namespace crynet::core