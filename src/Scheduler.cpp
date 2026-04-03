#include "crynet/Scheduler.hpp"

#include <stdexcept>

namespace crynet {

Scheduler::Scheduler(std::size_t num_threads)
    : num_threads_(num_threads > 0 ? num_threads : 1)
{}

Scheduler::~Scheduler() {
    // Ensure threads are joined even if the caller forgot to call stop()/run().
    stop();
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

// ---------------------------------------------------------------------------
// Actor management
// ---------------------------------------------------------------------------

ActorId Scheduler::alloc_id() noexcept {
    return next_id_.fetch_add(1, std::memory_order_relaxed);
}

ActorId Scheduler::spawn(std::unique_ptr<Actor> actor) {
    if (!actor) {
        throw std::invalid_argument("Scheduler::spawn: null actor");
    }
    ActorId id = actor->id();
    {
        std::lock_guard lock(actors_mutex_);
        actors_.emplace(id, std::move(actor));
    }
    return id;
}

// ---------------------------------------------------------------------------
// Message passing
// ---------------------------------------------------------------------------

void Scheduler::send(Message msg) {
    {
        std::lock_guard lock(queue_mutex_);
        work_queue_.push(std::move(msg));
    }
    queue_cv_.notify_one();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool Scheduler::running() const noexcept {
    return running_.load(std::memory_order_acquire);
}

void Scheduler::run() {
    running_.store(true, std::memory_order_release);

    threads_.reserve(num_threads_);
    for (std::size_t i = 0; i < num_threads_; ++i) {
        threads_.emplace_back([this] { worker_loop(); });
    }

    for (auto& t : threads_) {
        t.join();
    }
    threads_.clear();
}

void Scheduler::stop() {
    running_.store(false, std::memory_order_release);
    queue_cv_.notify_all();
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void Scheduler::worker_loop() {
    while (true) {
        Message msg;
        {
            std::unique_lock lock(queue_mutex_);
            queue_cv_.wait(lock, [this] {
                return !work_queue_.empty() || !running_.load(std::memory_order_acquire);
            });

            if (!running_.load(std::memory_order_acquire) && work_queue_.empty()) {
                return;
            }

            msg = std::move(work_queue_.front());
            work_queue_.pop();
        }
        dispatch(std::move(msg));
    }
}

void Scheduler::dispatch(Message msg) {
    std::lock_guard lock(actors_mutex_);
    auto it = actors_.find(msg.receiver);
    if (it != actors_.end()) {
        it->second->receive(msg);
    }
}

} // namespace crynet
