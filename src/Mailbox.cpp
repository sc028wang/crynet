#include "crynet/Mailbox.hpp"

#include <cassert>

namespace crynet {

Mailbox::Mailbox() {
    // Allocate a sentinel "dummy" node.  head_ points to the dummy;
    // tail_ points to the last real node (or the dummy when empty).
    auto* dummy = new Node{};
    head_        = dummy;
    tail_.store(dummy, std::memory_order_relaxed);
}

Mailbox::~Mailbox() {
    // Drain and free all remaining nodes (including the sentinel).
    while (head_ != nullptr) {
        Node* next = head_->next.load(std::memory_order_relaxed);
        delete head_;
        head_ = next;
    }
}

void Mailbox::push(Message msg) {
    // Allocate a new node off the heap.
    auto* node = new Node{std::move(msg)};

    // Atomically swing tail_ to the new node and link the old tail to it.
    // The exchange is acq_rel so that:
    //   * The store of node->next is visible before any subsequent pop().
    //   * The new node sees any prior stores from the producer.
    Node* prev = tail_.exchange(node, std::memory_order_acq_rel);
    prev->next.store(node, std::memory_order_release);
}

bool Mailbox::pop(Message& out) {
    Node* head = head_;                                          // sentinel
    Node* next = head->next.load(std::memory_order_acquire);    // first real node
    if (next == nullptr) {
        return false;   // queue is empty
    }

    // Move the payload out of *next, then advance head past the sentinel.
    // The old sentinel is deleted; next becomes the new sentinel.
    out   = std::move(next->msg);
    head_ = next;
    delete head;
    return true;
}

bool Mailbox::empty() const noexcept {
    return head_->next.load(std::memory_order_acquire) == nullptr;
}

} // namespace crynet
