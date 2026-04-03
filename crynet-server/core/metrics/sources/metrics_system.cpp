#include "core/metrics/includes/metrics_system.h"

namespace crynet::core {

void MetricsSystem::on_service_registered() noexcept {
    static_cast<void>(m_registered_services.fetch_add(1, std::memory_order_relaxed));
}

void MetricsSystem::on_message_enqueued() noexcept {
    static_cast<void>(m_enqueued_messages.fetch_add(1, std::memory_order_relaxed));
}

void MetricsSystem::on_message_dispatched() noexcept {
    static_cast<void>(m_dispatched_messages.fetch_add(1, std::memory_order_relaxed));
}

void MetricsSystem::on_timer_message() noexcept {
    static_cast<void>(m_timer_messages.fetch_add(1, std::memory_order_relaxed));
}

void MetricsSystem::on_message_dropped() noexcept {
    static_cast<void>(m_dropped_messages.fetch_add(1, std::memory_order_relaxed));
}

void MetricsSystem::on_queue_overload() noexcept {
    static_cast<void>(m_queue_overload_events.fetch_add(1, std::memory_order_relaxed));
}

void MetricsSystem::observe_queue_depth(std::size_t depth) noexcept {
    auto current_peak = m_peak_queue_depth.load(std::memory_order_relaxed);
    while (current_peak < depth &&
           !m_peak_queue_depth.compare_exchange_weak(
               current_peak,
               depth,
               std::memory_order_relaxed,
               std::memory_order_relaxed
           )) {
    }
}

MetricsSystemSnapshot MetricsSystem::snapshot() const noexcept {
    return MetricsSystemSnapshot{
        .registered_services = m_registered_services.load(std::memory_order_relaxed),
        .enqueued_messages = m_enqueued_messages.load(std::memory_order_relaxed),
        .dispatched_messages = m_dispatched_messages.load(std::memory_order_relaxed),
        .timer_messages = m_timer_messages.load(std::memory_order_relaxed),
        .dropped_messages = m_dropped_messages.load(std::memory_order_relaxed),
        .queue_overload_events = m_queue_overload_events.load(std::memory_order_relaxed),
        .peak_queue_depth = m_peak_queue_depth.load(std::memory_order_relaxed),
    };
}

}  // namespace crynet::core