# CryNet

> A modern, high-performance C++20 actor-model server framework inspired by [Skynet](https://github.com/cloudwu/skynet).

## Overview

CryNet brings Skynet's "everything is a service" philosophy to C++ while embracing the full power of **C++20**:

| Feature | Details |
|---|---|
| **Actor model** | Lightweight actors (services) communicate exclusively via message passing |
| **C++20 coroutines** | `co_await`-based bootstrap helpers; actor bodies can be coroutine-driven |
| **Concepts** | `ActorLike` concept enforces the actor interface at compile time |
| **Lock-free mailbox** | Per-actor MPSC queue (Dmitry Vyukov's algorithm) with no mutex contention on the hot path |
| **Thread pool** | Configurable worker threads; work queue protected by a single condition variable |
| **Type-safe payloads** | `std::any` data field – attach any value to a message without changing the wire format |

---

## Project structure

```
crynet/
├── include/crynet/
│   ├── Message.hpp      # Message struct
│   ├── Mailbox.hpp      # Lock-free MPSC queue
│   ├── Actor.hpp        # Abstract actor base class + ActorLike concept
│   └── Scheduler.hpp    # Thread-pool scheduler
├── src/
│   ├── Mailbox.cpp
│   ├── Actor.cpp
│   └── Scheduler.cpp
├── examples/
│   └── ping_pong.cpp    # Ping-Pong demo using C++20 coroutines
├── CMakeLists.txt
└── README.md
```

---

## Requirements

| Tool | Minimum version |
|---|---|
| CMake | 3.20 |
| C++ compiler | GCC 10 / Clang 12 / MSVC 19.29 (full C++20 support) |
| pthreads | Any POSIX platform (or native threads on Windows) |

---

## Build

```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build --parallel

# Run the ping-pong example
./build/ping_pong
```

Expected output:

```
[Bootstrap] sending start message to PingActor
[Ping] sending ping 1
[Pong] received ping from actor 1 – replying with pong
[Ping] received pong #1
[Ping] sending ping 2
[Pong] received ping from actor 1 – replying with pong
[Ping] received pong #2
...
[Ping] 5 rounds complete – shutting down.
Done.
```

---

## Quick-start: writing your own actor

```cpp
#include <crynet/Actor.hpp>
#include <crynet/Scheduler.hpp>
#include <iostream>

class HelloActor final : public crynet::Actor {
public:
    using Actor::Actor;

    void receive(const crynet::Message& msg) override {
        std::cout << "Hello from actor " << id() << "! type=" << msg.type << "\n";
        scheduler_.stop();
    }
};

int main() {
    crynet::Scheduler sched(2);

    crynet::ActorId id = sched.alloc_id();
    sched.spawn(std::make_unique<HelloActor>(id, sched));
    sched.send(crynet::Message{crynet::kNullId, id, "hello"});

    sched.run();
}
```

---

## Design notes

### Actor identity
Every actor is assigned a unique `ActorId` (a `uint64_t`).
Use `Scheduler::alloc_id()` to reserve an ID before constructing an actor,
which lets you pass cross-references between actors at construction time.

### Message routing
Messages are placed in a global work queue inside the scheduler.  Worker
threads dequeue messages and call the target actor's `receive()` in a thread
pool. For actors that require strict single-threaded execution, protect their
internal state with a `std::mutex`.

### Lock-free mailbox
`Mailbox` is an intrusive MPSC queue available for use when you want each actor
to buffer its own messages.  In the default scheduler design the global queue
plays this role, but `Mailbox` can be wired in as a per-actor buffer for
higher-throughput scenarios.

---

## License

MIT
