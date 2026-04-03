#include "core/scheduler/timer/includes/timer_queue.h"

#include <utility>

namespace crynet::core {

void TimerQueue::add_entry(Entry entry) {
    const auto expire_tick = entry.expire_tick;
    if ((expire_tick | k_near_mask) == (m_current_tick | k_near_mask)) {
        m_near[expire_tick & k_near_mask].push_back(std::move(entry));
        return;
    }

    std::uint64_t mask = k_near_size << k_level_shift;
    std::size_t level = 0;
    for (; level < k_level_count - 1; ++level) {
        if ((expire_tick | (mask - 1)) == (m_current_tick | (mask - 1))) {
            break;
        }
        mask <<= k_level_shift;
    }

    const auto index = static_cast<std::size_t>(
        (expire_tick >> (k_near_shift + level * k_level_shift)) & k_level_mask
    );
    m_levels[level][index].push_back(std::move(entry));
}

void TimerQueue::move_bucket(std::size_t level, std::size_t index) {
    auto& bucket = m_levels[level][index];
    while (!bucket.empty()) {
        auto entry = std::move(bucket.front());
        bucket.pop_front();
        add_entry(std::move(entry));
    }
}

void TimerQueue::advance_tick() {
    const auto current_tick = ++m_current_tick;
    if (current_tick == 0) {
        move_bucket(k_level_count - 1, 0);
        return;
    }

    std::uint64_t shifted_time = current_tick >> k_near_shift;
    std::uint64_t mask = k_near_size;
    for (std::size_t level = 0; level < k_level_count; ++level) {
        if ((current_tick & (mask - 1)) != 0) {
            break;
        }

        const auto index = static_cast<std::size_t>(shifted_time & k_level_mask);
        if (index != 0) {
            move_bucket(level, index);
            break;
        }

        mask <<= k_level_shift;
        shifted_time >>= k_level_shift;
    }
}

void TimerQueue::collect_current_bucket(std::vector<Message>& expired_messages) {
    auto& bucket = m_near[m_current_tick & k_near_mask];
    while (!bucket.empty()) {
        auto entry = std::move(bucket.front());
        bucket.pop_front();
        --m_pending_size;
        expired_messages.push_back(Message::from_text(
            MessageType::Timer,
            0,
            entry.target_handle,
            entry.session_id,
            entry.payload_text
        ));
    }
}

bool TimerQueue::remove_by_timer_id(TimerId timer_id) noexcept {
    for (auto& bucket : m_near) {
        for (auto entry = bucket.begin(); entry != bucket.end(); ++entry) {
            if (entry->timer_id == timer_id) {
                bucket.erase(entry);
                --m_pending_size;
                return true;
            }
        }
    }

    for (auto& level : m_levels) {
        for (auto& bucket : level) {
            for (auto entry = bucket.begin(); entry != bucket.end(); ++entry) {
                if (entry->timer_id == timer_id) {
                    bucket.erase(entry);
                    --m_pending_size;
                    return true;
                }
            }
        }
    }

    return false;
}

std::size_t TimerQueue::remove_by_target(ServiceHandle target_handle) noexcept {
    std::size_t removed = 0;

    for (auto& bucket : m_near) {
        for (auto entry = bucket.begin(); entry != bucket.end();) {
            if (entry->target_handle == target_handle) {
                entry = bucket.erase(entry);
                --m_pending_size;
                ++removed;
                continue;
            }
            ++entry;
        }
    }

    for (auto& level : m_levels) {
        for (auto& bucket : level) {
            for (auto entry = bucket.begin(); entry != bucket.end();) {
                if (entry->target_handle == target_handle) {
                    entry = bucket.erase(entry);
                    --m_pending_size;
                    ++removed;
                    continue;
                }
                ++entry;
            }
        }
    }

    return removed;
}

TimerId TimerQueue::schedule_after(
    ServiceHandle target_handle,
    SessionId session_id,
    std::chrono::milliseconds delay,
    std::string payload_text
) {
    const auto timer_id = m_next_timer_id++;
    const auto delay_ticks = delay.count() <= 0 ? 0ULL : static_cast<std::uint64_t>(delay.count());
    Entry entry{
        .timer_id = timer_id,
        .target_handle = target_handle,
        .session_id = session_id,
        .expire_tick = m_current_tick + delay_ticks,
        .payload_text = std::move(payload_text),
    };

    add_entry(std::move(entry));
    ++m_pending_size;

    return timer_id;
}

bool TimerQueue::cancel(TimerId timer_id) noexcept {
    return remove_by_timer_id(timer_id);
}

std::vector<Message> TimerQueue::poll_expired(std::chrono::steady_clock::time_point now) {
    std::vector<Message> expired_messages;
    if (now <= m_current_point) {
        return expired_messages;
    }

    const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_current_point).count();
    if (delta <= 0) {
        return expired_messages;
    }

    for (std::int64_t step = 0; step < delta; ++step) {
        collect_current_bucket(expired_messages);
        advance_tick();
        collect_current_bucket(expired_messages);
    }

    m_current_point += std::chrono::milliseconds{delta};
    return expired_messages;
}

std::size_t TimerQueue::size() const noexcept {
    return m_pending_size;
}

std::size_t TimerQueue::cancel_for_target(ServiceHandle target_handle) noexcept {
    return remove_by_target(target_handle);
}

}  // namespace crynet::core