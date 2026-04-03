#pragma once

#include "core/actor/includes/message.h"

#include <deque>
#include <optional>

namespace crynet::core {

/**
 * @cn
 * 单服务消息队列，负责按 FIFO 顺序缓存投递给指定服务的消息。
 *
 * @en
 * Per-service message queue that stores messages delivered to a specific service in FIFO order.
 */
class MessageQueue final {
public:
    /**
     * @cn
     * 使用队列所属服务句柄构造一个空消息队列。
     *
     * @en
     * Construct an empty message queue for the specified owner service handle.
     */
    explicit MessageQueue(ServiceHandle owner_handle) noexcept;

    /**
     * @cn
     * 返回当前队列所属服务句柄。
     *
     * @en
     * Return the owner service handle for the current queue.
     */
    [[nodiscard]] ServiceHandle owner_handle() const noexcept;

    /**
     * @cn
     * 将一条消息压入队列；只有目标句柄匹配所属服务时才会成功。
     *
     * @en
     * Push a message into the queue; succeeds only when the target handle matches the owner service.
     */
    [[nodiscard]] bool enqueue(Message message);

    /**
     * @cn
     * 以 FIFO 顺序弹出一条消息；若队列为空则返回空值。
     *
     * @en
     * Pop one message in FIFO order; returns no value when the queue is empty.
     */
    [[nodiscard]] std::optional<Message> dequeue() noexcept;

    /**
     * @cn
     * 判断当前队列是否为空。
     *
     * @en
     * Check whether the queue is currently empty.
     */
    [[nodiscard]] bool empty() const noexcept;

    /**
     * @cn
     * 返回当前队列中的消息数量。
     *
     * @en
     * Return the number of messages currently stored in the queue.
     */
    [[nodiscard]] std::size_t size() const noexcept;

private:
    /**
     * @cn
     * 当前队列绑定的服务句柄。
     *
     * @en
     * Service handle bound to the current queue.
     */
    ServiceHandle m_owner_handle{0};

    /**
     * @cn
     * 按 FIFO 顺序存储待处理消息的双端队列。
     *
     * @en
     * Double-ended queue storing pending messages in FIFO order.
     */
    std::deque<Message> m_messages;
};

}  // namespace crynet::core