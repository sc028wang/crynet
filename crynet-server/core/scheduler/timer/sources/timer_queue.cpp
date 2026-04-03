#include "core/scheduler/timer/includes/timer_queue.h"

#include <algorithm>
#include <utility>

namespace crynet::core {

TimerId TimerQueue::schedule_after(
    ServiceHandle target_handle,
    SessionId session_id,
    std::chrono::milliseconds delay,
    std::string payload_text
) {
    const auto timer_id = m_next_timer_id++;
    Entry entry{
        .timer_id = timer_id,
        .target_handle = target_handle,
        .session_id = session_id,
        .deadline = std::chrono::steady_clock::now() + delay,
        .payload_text = std::move(payload_text),
    };

    m_entries.push_back(std::move(entry));

    // Keep tasks sorted by deadline so expiration polling stays deterministic.
    std::ranges::sort(m_entries, [](const Entry& lhs, const Entry& rhs) {
        if (lhs.deadline == rhs.deadline) {
            return lhs.timer_id < rhs.timer_id;
        }

        return lhs.deadline < rhs.deadline;
    });

    return timer_id;
}

bool TimerQueue::cancel(TimerId timer_id) noexcept {
    const auto entry = std::ranges::find(m_entries, timer_id, &Entry::timer_id);
    if (entry == m_entries.end()) {
        return false;
    }

    m_entries.erase(entry);
    return true;
}

std::vector<Message> TimerQueue::poll_expired(std::chrono::steady_clock::time_point now) {
    std::vector<Message> expired_messages;

    auto split = m_entries.begin();
    while (split != m_entries.end() && split->deadline <= now) {
        expired_messages.push_back(Message::from_text(
            MessageType::Timer,
            0,
            split->target_handle,
            split->session_id,
            split->payload_text
        ));
        ++split;
    }

    m_entries.erase(m_entries.begin(), split);
    return expired_messages;
}

std::size_t TimerQueue::size() const noexcept {
    return m_entries.size();
}

}  // namespace crynet::core