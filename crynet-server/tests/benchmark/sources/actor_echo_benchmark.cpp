#include "tests/benchmark/includes/actor_echo_benchmark.h"

#include "core/actor/includes/actor_system.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

namespace crynet::tests::benchmark {

int ActorEchoBenchmark::run() noexcept {
    constexpr std::size_t k_request_count = 100000;
    constexpr std::size_t k_batch_size = 256;

    crynet::core::ActorSystem system;
    std::size_t response_count = 0;

    const auto caller_handle = system.register_service(
        "caller",
        [&response_count](crynet::core::ServiceContext&, const crynet::core::Message& message) {
            if (message.type() == crynet::core::MessageType::Response && message.text_payload() == "pong") {
                ++response_count;
            }
        }
    );
    const auto echo_handle = system.register_service(
        "echo",
        [&system](crynet::core::ServiceContext& context, const crynet::core::Message& message) {
            if (message.type() == crynet::core::MessageType::Request) {
                static_cast<void>(system.respond(context.handle(), message.session_id(), "pong"));
            }
        }
    );

    if (!caller_handle.has_value() || !echo_handle.has_value()) {
        std::cerr << "[benchmark] service registration failed" << std::endl;
        return 1;
    }

    for (std::size_t index = 0; index < k_request_count; ++index) {
        const auto session_id = system.call(caller_handle.value(), echo_handle.value(), "ping");
        if (!session_id.has_value()) {
            std::cerr << "[benchmark] request enqueue failed at index=" << index << std::endl;
            return 1;
        }
    }

    const auto begin = std::chrono::steady_clock::now();
    const auto dispatched_messages = system.run_until_idle(k_request_count * 2, k_batch_size);
    const auto end = std::chrono::steady_clock::now();

    const auto metrics = system.metrics();
    const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

    if (response_count != k_request_count || system.pending_sessions() != 0 ||
        dispatched_messages != k_request_count * 2 || metrics.enqueued_messages != k_request_count * 2 ||
        metrics.dispatched_messages != k_request_count * 2) {
        std::cerr << "[benchmark] actor echo mismatch"
                  << " responses=" << response_count
                  << " pending_sessions=" << system.pending_sessions()
                  << " dispatched=" << dispatched_messages
                  << " metrics_enqueued=" << metrics.enqueued_messages
                  << " metrics_dispatched=" << metrics.dispatched_messages
                  << std::endl;
        return 1;
    }

    std::cout << "[benchmark] actor_echo requests=" << k_request_count
              << " total_messages=" << (k_request_count * 2)
              << " elapsed_us=" << elapsed_us
              << " peak_queue_depth=" << metrics.peak_queue_depth
              << std::endl;
    return 0;
}

}  // namespace crynet::tests::benchmark

int main() {
    return crynet::tests::benchmark::ActorEchoBenchmark::run();
}