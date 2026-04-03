#pragma once

#include "core/actor/includes/actor_system.h"
#include "core/scheduler/includes/worker_plugin.h"

#include <functional>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace crynet::core {

/**
 * @cn
 * worker monitor 快照，用于观察 sleep/wake/quit 相关状态。
 *
 * @en
 * Worker-monitor snapshot used to observe sleep, wake, and quit-related state.
 */
struct WorkerMonitorSnapshot final {
    /**
     * @cn
     * 当前被 monitor 管理的 worker 数量。
     *
     * @en
     * Number of workers currently managed by the monitor.
     */
    std::size_t worker_count{0};

    /**
     * @cn
     * 当前处于 sleeping 状态的 worker 数量。
     *
     * @en
     * Number of workers currently in the sleeping state.
     */
    std::size_t sleeping_workers{0};

    /**
     * @cn
     * 累计发生的 wake 事件数量。
     *
     * @en
     * Cumulative number of wake events that have occurred.
     */
    std::size_t wake_events{0};

    /**
     * @cn
     * 累计发生的 busy 调度轮次数量。
     *
     * @en
     * Cumulative number of busy dispatch turns that have occurred.
     */
    std::size_t busy_turns{0};

    /**
     * @cn
     * 累计检测到的疑似卡死事件数量。
     *
     * @en
     * Cumulative number of suspected stall events that have been detected.
     */
    std::size_t stall_events{0};

    /**
     * @cn
     * 是否已经收到退出请求。
     *
     * @en
     * Whether a quit request has been observed.
     */
    bool quit_requested{false};
};

/**
 * @cn
 * worker 当前观察到的活动路由。
 *
 * @en
 * Active route currently observed for one worker.
 */
struct WorkerRoute final {
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
     * 当前消息源服务。
     *
     * @en
     * Source service of the current message.
     */
    ServiceHandle source_handle{0};

    /**
     * @cn
     * 当前消息目标服务。
     *
     * @en
     * Target service of the current message.
     */
    ServiceHandle target_handle{0};

    /**
     * @cn
     * 当前消息会话编号。
     *
     * @en
     * Session identifier of the current message.
     */
    SessionId session_id{0};

    /**
     * @cn
     * 判断当前路由是否处于激活状态。
     *
     * @en
     * Check whether the current route is active.
     */
    [[nodiscard]] bool active() const noexcept {
        return target_handle != 0;
    }
};

/**
 * @cn
 * worker monitor 产生的告警事件。
 *
 * @en
 * Alert event emitted by the worker monitor.
 */
struct WorkerMonitorAlert final {
    /**
     * @cn
     * 触发告警的 worker 编号。
     *
     * @en
     * Identifier of the worker that triggered the alert.
     */
    std::size_t worker_id{0};

    /**
     * @cn
     * 触发告警时观察到的活动路由。
     *
     * @en
     * Active route observed when the alert was triggered.
     */
    WorkerRoute route;

    /**
     * @cn
     * 告警消息文本。
     *
     * @en
     * Alert message text.
     */
    std::string message;
};

/**
 * @cn
 * worker monitor 告警回调类型。
 *
 * @en
 * Alert callback type used by the worker monitor.
 */
using WorkerMonitorAlertSink = std::function<void(const WorkerMonitorAlert&)>;

/**
 * @cn
 * 最小 worker monitor，负责记录 worker 的睡眠、唤醒和退出状态。
 *
 * @en
 * Minimal worker monitor that records worker sleep, wake, and quit state.
 */
class WorkerMonitor final : public WorkerPlugin {
public:
    /**
     * @cn
     * 创建一个 monitor，并指定需要管理的 worker 数量。
     *
     * @en
     * Create a monitor and specify how many workers it should manage.
     */
    explicit WorkerMonitor(std::size_t worker_count);

    /**
     * @cn
     * 将指定 worker 标记为 sleeping。
     *
     * @en
     * Mark the specified worker as sleeping.
     */
    void mark_sleeping(std::size_t worker_id) noexcept;

    /**
     * @cn
     * 将指定 worker 标记为 awake；若此前处于 sleeping，则累计一次 wake 事件。
     *
     * @en
     * Mark the specified worker as awake; if it was sleeping previously,
     * accumulate one wake event.
     */
    void mark_awake(std::size_t worker_id) noexcept;

    /**
     * @cn
     * 记录指定 worker 完成了一次 busy 调度轮次。
     *
     * @en
     * Record that the specified worker completed one busy dispatch turn.
     */
    void on_busy_turn(std::size_t worker_id) noexcept;

    /**
     * @cn
     * 请求 monitor 进入退出状态。
     *
     * @en
     * Request that the monitor enter the quit state.
     */
    void request_quit() noexcept;

    /**
     * @cn
     * 返回当前是否已经收到退出请求。
     *
     * @en
     * Return whether a quit request has already been observed.
     */
    [[nodiscard]] bool quit_requested() const noexcept;

    /**
     * @cn
     * 判断在给定 busy worker 数量下是否应尝试唤醒 sleeping worker。
     *
     * @en
     * Determine whether a sleeping worker should be woken under the specified busy-worker count.
     */
    [[nodiscard]] bool should_wake(std::size_t busy_workers) const noexcept;

    /**
     * @cn
     * 返回当前 monitor 的状态快照。
     *
     * @en
     * Return a snapshot of the current monitor state.
     */
    [[nodiscard]] WorkerMonitorSnapshot snapshot() const noexcept;

    /**
     * @cn
     * 判断插件是否请求当前 worker 停止继续执行。
     *
     * @en
     * Check whether the plugin requests the current worker to stop further execution.
     */
    [[nodiscard]] bool stop_requested(std::size_t worker_id) const noexcept override;

    /**
     * @cn
     * 在消息分发开始前更新 monitor 当前活动路由。
     *
     * @en
     * Update the current active route in the monitor before message dispatch begins.
     */
    void on_dispatch_begin(std::size_t worker_id, const DispatchTrace& trace) noexcept override;

    /**
     * @cn
     * 在非空分发完成后更新 awake、busy 和 route 清理状态。
     *
     * @en
     * Update awake, busy, and route-cleanup state after a non-idle dispatch turn completes.
     */
    void on_dispatch_end(std::size_t worker_id) noexcept override;

    /**
     * @cn
     * 在空闲轮次中将当前 worker 标记为 sleeping。
     *
     * @en
     * Mark the current worker as sleeping during an idle turn.
     */
    void on_idle(std::size_t worker_id) noexcept override;

    /**
     * @cn
     * 设置 stall 告警回调；检测到疑似卡死时会同步调用。
     *
     * @en
     * Set the stall alert callback, invoked synchronously when a suspected stall is detected.
     */
    void set_alert_sink(WorkerMonitorAlertSink sink);

    /**
     * @cn
     * 清除自定义 stall 告警回调。
     *
     * @en
     * Clear the custom stall alert callback.
     */
    void reset_alert_sink() noexcept;

    /**
     * @cn
     * 启用或关闭默认 logger 告警输出。
     *
     * @en
     * Enable or disable default logger-backed alert emission.
     */
    void set_logger_alert_enabled(bool enabled) noexcept;

    /**
     * @cn
     * 记录当前 worker 即将执行的消息路由。
     *
     * @en
     * Record the message route that the current worker is about to execute.
     */
    void trigger(std::size_t worker_id, const DispatchTrace& trace) noexcept;

    /**
     * @cn
     * 清除当前 worker 的活动路由，表示本轮执行已完成。
     *
     * @en
     * Clear the active route of the current worker, indicating that the current turn has completed.
     */
    void clear(std::size_t worker_id) noexcept;

    /**
     * @cn
     * 返回当前 worker 的活动路由；若没有活动消息则返回空值。
     *
     * @en
     * Return the current active route of the worker, or no value when no message is active.
     */
    [[nodiscard]] std::optional<WorkerRoute> active_route(std::size_t worker_id) const noexcept;

    /**
     * @cn
     * 检查当前 worker 是否在相同活动路由上没有推进，若疑似卡死则返回该路由。
     *
     * @en
     * Check whether the current worker has made no progress on the same active route;
     * return that route when a suspected stall is detected.
     */
    [[nodiscard]] std::optional<WorkerRoute> check_stalled(std::size_t worker_id) noexcept;

private:
    /**
     * @cn
     * 单个 worker 的 monitor 槽位。
     *
     * @en
     * Monitor slot for one worker.
     */
    struct WorkerSlot final {
        /**
         * @cn
         * 当前 worker 是否处于 sleeping 状态。
         *
         * @en
         * Whether the current worker is in the sleeping state.
         */
        bool sleeping{false};

        /**
         * @cn
         * 当前活动路由。
         *
         * @en
         * Current active route.
         */
        WorkerRoute route;

        /**
         * @cn
         * 活动路由版本号，每次 trigger 或 clear 时都会推进。
         *
         * @en
         * Active-route version, advanced on every trigger or clear.
         */
        std::size_t version{0};

        /**
         * @cn
         * 上次 stall 检查记录的版本号。
         *
         * @en
         * Version recorded during the previous stall check.
         */
        std::size_t checked_version{0};
    };

    /**
     * @cn
     * 判断给定 worker 是否在合法范围内。
     *
     * @en
     * Check whether the specified worker id is within range.
     */
    [[nodiscard]] bool contains_worker(std::size_t worker_id) const noexcept;

    /**
     * @cn
     * 组装并发出一次 stall 告警。
     *
     * @en
     * Build and emit one stall alert.
     */
    void emit_stall_alert(std::size_t worker_id, const WorkerRoute& route);

    /**
     * @cn
     * 每个 worker 的 sleeping 状态。
     *
     * @en
     * Sleeping state of each worker.
     */
    std::vector<WorkerSlot> m_workers;

    /**
     * @cn
     * 当前 sleeping worker 数量。
     *
     * @en
     * Current number of sleeping workers.
     */
    std::size_t m_sleeping_workers{0};

    /**
     * @cn
     * 累计 wake 事件数量。
     *
     * @en
     * Cumulative number of wake events.
     */
    std::size_t m_wake_events{0};

    /**
     * @cn
     * 累计 busy 调度轮次数量。
     *
     * @en
     * Cumulative number of busy dispatch turns.
     */
    std::size_t m_busy_turns{0};

    /**
     * @cn
     * 累计检测到的疑似卡死事件数量。
     *
     * @en
     * Cumulative number of suspected stall events.
     */
    std::size_t m_stall_events{0};

    /**
     * @cn
     * 自定义 stall 告警回调。
     *
     * @en
     * Custom stall alert callback.
     */
    WorkerMonitorAlertSink m_alert_sink;

    /**
     * @cn
     * 是否启用默认 logger 告警输出。
     *
     * @en
     * Whether default logger-backed alert emission is enabled.
     */
    bool m_logger_alert_enabled{true};

    /**
     * @cn
     * 是否已经收到退出请求。
     *
     * @en
     * Whether a quit request has already been observed.
     */
    bool m_quit_requested{false};
};

}  // namespace crynet::core