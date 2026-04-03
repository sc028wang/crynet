#pragma once

#include "Actor.hpp"
#include "Message.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace crynet {

// ---------------------------------------------------------------------------
// Scheduler – the heart of CryNet.
//
// The scheduler owns all registered actors and drives message delivery across
// a configurable pool of worker threads.
//
// Design
// ------
// * A global work-queue holds (actor-id, message) work items.
// * Worker threads compete on this queue using a condition variable.
// * Each work item invokes the target actor's `receive()` exactly once.
// * Because multiple workers may be running concurrently, actors that require
//   single-threaded execution should protect their state with a mutex, or use
//   the per-actor Mailbox + scheduled-flag pattern documented in Mailbox.hpp.
//
// Usage
// -----
//   Scheduler sched(4);               // 4 worker threads
//   ActorId a = sched.spawn(...);     // register an actor
//   sched.send(Message{...});         // enqueue a message
//   sched.run();                      // block until stop() is called
// ---------------------------------------------------------------------------
class Scheduler {
public:
    explicit Scheduler(
        std::size_t num_threads = std::thread::hardware_concurrency());

    ~Scheduler();

    // Non-copyable / non-movable.
    Scheduler(const Scheduler&)            = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    // -----------------------------------------------------------------------
    // Actor management
    // -----------------------------------------------------------------------

    /// Register a pre-constructed actor.  Returns the actor's id.
    /// The scheduler takes ownership.
    ActorId spawn(std::unique_ptr<Actor> actor);

    /// Allocate a fresh, monotonically-increasing actor id.
    /// Actors must be constructed with this id before calling spawn().
    [[nodiscard]] ActorId alloc_id() noexcept;

    // -----------------------------------------------------------------------
    // Message passing
    // -----------------------------------------------------------------------

    /// Enqueue `msg` for delivery to msg.receiver.
    /// Thread-safe; may be called from any thread.
    void send(Message msg);

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /// Start worker threads and block until stop() is called.
    void run();

    /// Signal all worker threads to exit after draining the current queue.
    void stop();

    /// Returns true while the scheduler is running.
    [[nodiscard]] bool running() const noexcept;

private:
    void worker_loop();
    void dispatch(Message msg);

    // Actor registry
    std::unordered_map<ActorId, std::unique_ptr<Actor>> actors_;
    mutable std::mutex                                   actors_mutex_;

    // Global message queue consumed by worker threads
    std::queue<Message>     work_queue_;
    std::mutex              queue_mutex_;
    std::condition_variable queue_cv_;

    // State
    std::atomic<bool>        running_{false};
    std::atomic<ActorId>     next_id_{1};
    std::size_t              num_threads_;
    std::vector<std::thread> threads_;
};

} // namespace crynet
