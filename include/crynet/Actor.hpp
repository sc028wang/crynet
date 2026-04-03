#pragma once

#include "Message.hpp"
#include "Mailbox.hpp"

#include <any>
#include <concepts>
#include <string>

namespace crynet {

class Scheduler; // forward declaration – breaks the circular dependency

// ---------------------------------------------------------------------------
// Concept: anything that looks like an Actor (duck-typed interface).
// Useful for generic helpers that accept any actor-like object.
// ---------------------------------------------------------------------------
template <typename T>
concept ActorLike = requires(T a, const Message& m) {
    { a.id() }       -> std::convertible_to<ActorId>;
    { a.receive(m) } -> std::same_as<void>;
};

// ---------------------------------------------------------------------------
// Base class for all CryNet actors (services).
//
// Lifecycle
// ---------
//  1. Construct with a pre-allocated ID and a reference to the owning
//     Scheduler.
//  2. Register with `Scheduler::spawn()`.
//  3. The scheduler calls `receive()` whenever a message is dispatched to
//     this actor's ID.
//  4. The actor can send messages to others via the `send()` helper.
// ---------------------------------------------------------------------------
class Actor {
public:
    explicit Actor(ActorId id, Scheduler& sched)
        : id_(id), scheduler_(sched)
    {}

    virtual ~Actor() = default;

    // Non-copyable (actors are identity-bearing objects).
    Actor(const Actor&)            = delete;
    Actor& operator=(const Actor&) = delete;

    /// Return this actor's unique identifier.
    [[nodiscard]] ActorId id() const noexcept { return id_; }

    /// Process an incoming message.  Overridden by each concrete actor.
    virtual void receive(const Message& msg) = 0;

    /// Deliver a message directly into this actor's mailbox.
    /// Called by the scheduler; not normally used directly by actors.
    void deliver(Message msg);

    /// Access the per-actor mailbox (used by the scheduler).
    [[nodiscard]] Mailbox& mailbox() noexcept { return mailbox_; }

protected:
    /// Send a message from this actor to another actor identified by `target`.
    void send(ActorId target, std::string type, std::any data = {});

    ActorId    id_;
    Scheduler& scheduler_;
    Mailbox    mailbox_;
};

} // namespace crynet
