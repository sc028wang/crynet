/// ping_pong.cpp – CryNet "Hello World" example
///
/// Demonstrates the actor model:
///  • PingActor sends a "ping" message to PongActor.
///  • PongActor replies with a "pong" message.
///  • After MAX_ROUNDS exchanges the scheduler is stopped.
///
/// C++20 Coroutine showcase
/// ------------------------
/// A simple fire-and-forget Task coroutine is used to schedule the very first
/// ping asynchronously after the scheduler has started, without blocking the
/// main thread.

#include <crynet/Actor.hpp>
#include <crynet/Message.hpp>
#include <crynet/Scheduler.hpp>

#include <coroutine>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>

// ---------------------------------------------------------------------------
// Minimal C++20 fire-and-forget coroutine task
// ---------------------------------------------------------------------------
struct Task {
    struct promise_type {
        Task get_return_object() noexcept { return {}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend()   noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };
};

// A simple awaitable that resumes the coroutine after `ms` milliseconds.
struct SleepFor {
    std::chrono::milliseconds ms;

    bool await_ready() const noexcept { return ms.count() <= 0; }

    void await_suspend(std::coroutine_handle<> h) const {
        std::thread([h, duration = ms] {
            std::this_thread::sleep_for(duration);
            h.resume();
        }).detach();
    }

    void await_resume() const noexcept {}
};

// ---------------------------------------------------------------------------
// Actors
// ---------------------------------------------------------------------------

static constexpr int MAX_ROUNDS = 5;

class PongActor final : public crynet::Actor {
public:
    using Actor::Actor;

    void receive(const crynet::Message& msg) override {
        if (msg.type == "ping") {
            std::cout << "[Pong] received ping from actor " << msg.sender
                      << " – replying with pong\n";
            send(msg.sender, "pong");
        }
    }
};

class PingActor final : public crynet::Actor {
public:
    PingActor(crynet::ActorId id, crynet::Scheduler& sched,
              crynet::ActorId pong_id)
        : Actor(id, sched), pong_id_(pong_id)
    {}

    void receive(const crynet::Message& msg) override {
        if (msg.type == "start" || msg.type == "pong") {
            if (msg.type == "pong") {
                ++rounds_;
                std::cout << "[Ping] received pong #" << rounds_ << "\n";
            }

            if (rounds_ < MAX_ROUNDS) {
                std::cout << "[Ping] sending ping " << (rounds_ + 1) << "\n";
                send(pong_id_, "ping");
            } else {
                std::cout << "[Ping] " << MAX_ROUNDS
                          << " rounds complete – shutting down.\n";
                scheduler_.stop();
            }
        }
    }

private:
    crynet::ActorId pong_id_;
    int             rounds_{0};
};

// ---------------------------------------------------------------------------
// Coroutine bootstrap: wait briefly then kick off the first message.
// ---------------------------------------------------------------------------
Task bootstrap(crynet::Scheduler& sched, crynet::ActorId ping_id) {
    // Give the scheduler a moment to start all worker threads.
    co_await SleepFor{std::chrono::milliseconds(50)};

    std::cout << "[Bootstrap] sending start message to PingActor\n";
    sched.send(crynet::Message{crynet::kNullId, ping_id, "start"});
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    crynet::Scheduler sched(2);   // 2 worker threads

    // Allocate IDs before constructing the actors (so we can cross-reference).
    crynet::ActorId ping_id = sched.alloc_id();
    crynet::ActorId pong_id = sched.alloc_id();

    sched.spawn(std::make_unique<PingActor>(ping_id, sched, pong_id));
    sched.spawn(std::make_unique<PongActor>(pong_id, sched));

    // Kick off the exchange via a C++20 coroutine.
    bootstrap(sched, ping_id);

    // Block until stop() is called (from inside PingActor::receive).
    sched.run();

    std::cout << "Done.\n";
    return 0;
}
