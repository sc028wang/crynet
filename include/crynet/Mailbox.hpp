#pragma once

#include "Message.hpp"

#include <atomic>
#include <cstddef>

namespace crynet {

/// Lock-free, multiple-producer / single-consumer (MPSC) message queue.
///
/// Algorithm: Dmitry Vyukov's intrusive MPSC queue.
/// * `push()` is safe to call concurrently from many threads.
/// * `pop()` must only be called from one thread at a time (the actor's own
///   execution context).
///
/// Memory is managed with plain `new`/`delete`; for high-throughput scenarios
/// replace with a pool allocator.
class Mailbox {
    struct Node {
        Message            msg;
        std::atomic<Node*> next{nullptr};
    };

    // Producers update `tail_`; consumer owns `head_`.
    // Place on separate cache lines to avoid false sharing.
    alignas(64) std::atomic<Node*> tail_;
    alignas(64) Node*              head_;

public:
    Mailbox();
    ~Mailbox();

    // Non-copyable / non-movable (contains raw pointers that own memory).
    Mailbox(const Mailbox&)            = delete;
    Mailbox& operator=(const Mailbox&) = delete;

    /// Enqueue a message.  Thread-safe; may be called from any thread.
    void push(Message msg);

    /// Dequeue a message into `out`.
    /// Returns true on success, false when the queue is empty.
    /// NOT thread-safe: must only be called by the single consumer.
    bool pop(Message& out);

    /// Peek whether the queue is empty.
    /// NOT thread-safe: must only be called by the single consumer.
    [[nodiscard]] bool empty() const noexcept;
};

} // namespace crynet
