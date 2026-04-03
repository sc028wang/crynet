#include "crynet/Actor.hpp"
#include "crynet/Scheduler.hpp"

namespace crynet {

void Actor::deliver(Message msg) {
    mailbox_.push(std::move(msg));
}

void Actor::send(ActorId target, std::string type, std::any data) {
    scheduler_.send(Message{id_, target, std::move(type), std::move(data)});
}

} // namespace crynet
