#pragma once

#include "core/actor/includes/handle_registry.h"
#include "core/actor/includes/message_queue.h"
#include "core/actor/includes/session_manager.h"
#include "core/metrics/includes/metrics_system.h"
#include "core/scheduler/includes/worker_scheduler.h"
#include "core/scheduler/timer/includes/timer_queue.h"

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace crynet::core {

/**
 * @cn
 * 服务消息回调类型，用于在调度器驱动下执行单次消息处理。
 *
 * @en
 * Service-message callback type used to execute one message-processing turn under scheduler control.
 */
using MessageHandler = std::function<void(ServiceContext&, const Message&)>;

/**
 * @cn
 * 单次消息分发的观测记录，用于把当前路由暴露给调度外部组件。
 *
 * @en
 * Observation record for one message dispatch, used to expose the active route to external scheduler helpers.
 */
struct DispatchTrace final {
    /**
     * @cn
     * 当前消息类型。
     *
     * @en
     * Type of the current message.
     */
    MessageType message_type{MessageType::Invalid};

    /**
     * @cn
     * 当前消息的源服务句柄。
     *
     * @en
     * Source service handle of the current message.
     */
    ServiceHandle source_handle{0};

    /**
     * @cn
     * 当前消息的目标服务句柄。
     *
     * @en
     * Target service handle of the current message.
     */
    ServiceHandle target_handle{0};

    /**
     * @cn
     * 当前消息关联的会话编号。
     *
     * @en
     * Session identifier associated with the current message.
     */
    SessionId session_id{0};
};

/**
 * @cn
 * 分发观测回调类型，用于在消息执行前观察当前路由。
 *
 * @en
 * Dispatch-observer callback type used to observe the active route before message execution.
 */
using DispatchObserver = std::function<void(const DispatchTrace&)>;

/**
 * @cn
 * 最小 Actor 系统骨架，负责连接服务注册、消息投递、调度执行和超时注入。
 *
 * @en
 * Minimal actor-system skeleton that connects service registration, message delivery,
 * scheduler-driven execution, and timeout injection.
 */
class ActorSystem final {
public:
    /**
     * @cn
     * 注册一个服务，并返回分配的服务句柄；名称重复时返回空值。
     *
     * @en
     * Register a service and return the allocated service handle; returns no value on duplicate names.
     */
    [[nodiscard]] std::optional<ServiceHandle> register_service(std::string service_name, MessageHandler handler);

    /**
     * @cn
     * 向目标服务投递一条消息，并在成功后将其标记为可调度。
     *
     * @en
     * Deliver a message to the target service and mark it runnable after successful enqueue.
     */
    [[nodiscard]] bool send(Message message);

    /**
     * @cn
     * 停止指定服务并将其置为 `Stopped` 状态。
     *
     * @en
     * Stop the specified service and move it into the `Stopped` state.
     */
    [[nodiscard]] bool stop_service(ServiceHandle handle) noexcept;

    /**
     * @cn
     * 释放指定服务，清理队列、会话和定时器，并移出系统注册表。
     *
     * @en
     * Release the specified service, clean up queues, sessions, and timers, and remove it from the system registry.
     */
    [[nodiscard]] bool release_service(ServiceHandle handle) noexcept;

    /**
     * @cn
     * 停止并释放系统中的所有服务，返回本次移除的服务数量。
     *
     * @en
     * Stop and release all services in the system and return the number removed in this pass.
     */
    [[nodiscard]] std::size_t shutdown() noexcept;

    /**
     * @cn
     * 发起一次请求调用，自动分配会话编号并投递 `Request` 消息。
     *
     * @en
     * Start a request call, automatically allocate a session identifier, and deliver a `Request` message.
     */
    [[nodiscard]] std::optional<SessionId> call(
        ServiceHandle source_handle,
        ServiceHandle target_handle,
        std::string payload_text
    );

    /**
     * @cn
     * 针对既有会话发送响应消息，并在成功后关闭该会话。
     *
     * @en
     * Send a response message for an existing session and close the session on success.
     */
    [[nodiscard]] bool respond(
        ServiceHandle source_handle,
        SessionId session_id,
        std::string payload_text
    );

    /**
     * @cn
     * 为目标服务注册一个超时任务，并返回对应的定时器编号。
     *
     * @en
     * Register a timeout task for the target service and return the corresponding timer identifier.
     */
    [[nodiscard]] TimerId schedule_timeout(
        ServiceHandle target_handle,
        SessionId session_id,
        std::chrono::milliseconds delay,
        std::string payload_text = "timeout"
    );

    /**
     * @cn
     * 取消一个尚未触发的超时任务。
     *
     * @en
     * Cancel a timeout task that has not yet fired.
     */
    [[nodiscard]] bool cancel_timeout(TimerId timer_id) noexcept;

    /**
     * @cn
     * 轮询超时队列，并将到期消息注入系统消息流。
     *
     * @en
     * Poll the timeout queue and inject expired messages into the system message flow.
     */
    [[nodiscard]] std::size_t pump_timers(std::chrono::steady_clock::time_point now);

    /**
     * @cn
     * 执行一次批量调度，最多消费某个服务的指定消息数，并返回本轮处理总数。
     *
     * @en
     * Execute one batched dispatch turn, consuming up to the specified number of messages from one service,
     * and return the number processed in this turn.
     */
    [[nodiscard]] std::size_t dispatch_batch(
        std::size_t max_messages_per_service = 1,
        DispatchObserver observer = {}
    );

    /**
     * @cn
     * 持续调度直到系统空闲或达到批次数上限，并返回累计处理消息数。
     *
     * @en
     * Continue dispatching until the system becomes idle or the batch limit is reached,
     * and return the cumulative number of processed messages.
     */
    [[nodiscard]] std::size_t run_until_idle(
        std::size_t max_batches = 64,
        std::size_t max_messages_per_service = 1
    );

    /**
     * @cn
     * 执行一次调度循环，处理一条消息；若本轮没有可执行工作则返回 `false`。
     *
     * @en
     * Execute one scheduler turn and process a single message; returns `false` when no work is available.
     */
    [[nodiscard]] bool dispatch_once();

    /**
     * @cn
     * 返回当前已注册的服务数量。
     *
     * @en
     * Return the number of services currently registered in the system.
     */
    [[nodiscard]] std::size_t service_count() const noexcept;

    /**
     * @cn
     * 根据服务名称查询已注册句柄。
     *
     * @en
     * Resolve the registered handle by service name.
     */
    [[nodiscard]] std::optional<ServiceHandle> find_service_handle(std::string_view service_name) const;

    /**
     * @cn
     * 根据句柄查询当前服务生命周期状态。
     *
     * @en
     * Query the current lifecycle state of a service by handle.
     */
    [[nodiscard]] std::optional<ServiceState> service_state(ServiceHandle handle) const noexcept;

    /**
     * @cn
     * 返回当前挂起中的请求会话数量。
     *
     * @en
     * Return the number of request sessions currently pending.
     */
    [[nodiscard]] std::size_t pending_sessions() const noexcept;

    /**
     * @cn
     * 返回指定服务待处理消息数量。
     *
     * @en
     * Return the number of pending messages buffered for the specified service.
     */
    [[nodiscard]] std::size_t pending_messages(ServiceHandle handle) const noexcept;

    /**
     * @cn
    * 返回当前 Actor 系统的指标系统快照。
     *
     * @en
    * Return a metrics-system snapshot for the current actor system.
     */
    [[nodiscard]] MetricsSystemSnapshot metrics() const noexcept;

private:
    /**
     * @cn
     * 单个服务在系统中的内部槽位。
     *
     * @en
     * Internal system slot describing one registered service.
     */
    struct ServiceSlot final {
        /**
         * @cn
         * 当前服务的上下文对象。
         *
         * @en
         * Context object of the current service.
         */
        ServiceContext context;

        /**
         * @cn
         * 当前服务绑定的单独消息队列。
         *
         * @en
         * Dedicated message queue bound to the current service.
         */
        MessageQueue queue;

        /**
         * @cn
         * 当前服务的消息处理回调。
         *
         * @en
         * Message-processing callback of the current service.
         */
        MessageHandler handler;
    };

    /**
     * @cn
     * 服务句柄注册表，用于维护系统身份映射。
     *
     * @en
     * Service-handle registry maintaining system identity mappings.
     */
    HandleRegistry m_handle_registry;

    /**
     * @cn
     * 会话管理器，用于维护请求-响应关联关系。
     *
     * @en
     * Session manager used to maintain request-response correlations.
     */
    SessionManager m_session_manager;

    /**
     * @cn
     * 调度器，用于驱动服务执行顺序。
     *
     * @en
     * Scheduler responsible for driving service execution order.
     */
    WorkerScheduler m_scheduler;

    /**
     * @cn
     * 超时队列，用于生成 `Timer` 消息。
     *
     * @en
     * Timeout queue used to generate `Timer` messages.
     */
    TimerQueue m_timer_queue;

    /**
     * @cn
     * 服务句柄到系统槽位的映射表。
     *
     * @en
     * Mapping table from service handles to system service slots.
     */
    std::unordered_map<ServiceHandle, ServiceSlot> m_services;

    /**
     * @cn
    * 系统级指标系统累计器。
     *
     * @en
     * System-wide metrics-system accumulator.
     */
    MetricsSystem m_metrics;
};

}  // namespace crynet::core