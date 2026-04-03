#pragma once

#include <any>
#include <cstdint>
#include <string>

namespace crynet {

/// Unique identifier for every actor (service) in the system.
using ActorId = std::uint64_t;

/// Reserved "null" actor id – no valid actor is assigned this value.
inline constexpr ActorId kNullId = 0;

/// A message passed between actors.
///
/// Actors communicate exclusively through Message objects; there is no shared
/// state.  The `data` field carries an arbitrary payload wrapped in std::any,
/// so callers can attach any copyable/movable value without changing the
/// message wire format.
struct Message {
    ActorId     sender{kNullId};    ///< originating actor (0 = system/anonymous)
    ActorId     receiver{kNullId};  ///< destination actor
    std::string type;               ///< message discriminator (e.g. "ping", "pong")
    std::any    data;               ///< optional payload

    Message() = default;

    Message(ActorId sender, ActorId receiver, std::string type,
            std::any data = {})
        : sender(sender)
        , receiver(receiver)
        , type(std::move(type))
        , data(std::move(data))
    {}
};

} // namespace crynet
