#include "tests/unit/includes/smoke_suite.h"

#include "core/actor/includes/message.h"
#include "core/actor/includes/actor_system.h"
#include "core/bootstrap/includes/bootstrap_config.h"
#include "core/bootstrap/includes/bootstrapper.h"
#include "core/actor/includes/handle_registry.h"
#include "core/base/includes/logger.h"
#include "core/monitor/includes/monitor_runner.h"
#include "core/metrics/includes/metrics_system.h"
#include "core/actor/includes/message_queue.h"
#include "core/actor/includes/session_manager.h"
#include "core/actor/includes/service_context.h"
#include "core/scheduler/includes/ready_queue.h"
#include "core/scheduler/includes/worker_loop.h"
#include "core/scheduler/includes/worker_monitor.h"
#include "core/scheduler/includes/worker_runner.h"
#include "core/scheduler/includes/worker_scheduler.h"
#include "core/scheduler/timer/includes/timer_queue.h"
#include "startup/app/includes/startup_app.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace crynet::tests::unit {

namespace {

/**
 * @cn
 * 验证最小消息模型的元数据和文本载荷行为是否符合预期。
 *
 * @en
 * Validate that the minimal message model preserves metadata and text payload behavior.
 */
bool verify_message_model() {
    const auto message = crynet::core::Message::from_text(
        crynet::core::MessageType::Request,
        1001,
        2001,
        3001,
        "boot"
    );

    if (message.type() != crynet::core::MessageType::Request) {
        std::cerr << "[unit] message type mismatch" << std::endl;
        return false;
    }

    if (crynet::core::message_type_name(message.type()) != "request") {
        std::cerr << "[unit] message type name mismatch" << std::endl;
        return false;
    }

    if (message.source_handle() != 1001 || message.target_handle() != 2001) {
        std::cerr << "[unit] message routing metadata mismatch" << std::endl;
        return false;
    }

    if (message.session_id() != 3001) {
        std::cerr << "[unit] message session mismatch" << std::endl;
        return false;
    }

    if (message.empty() || message.payload_size() != 4 || message.text_payload() != "boot") {
        std::cerr << "[unit] message payload mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证最小服务上下文的生命周期状态流转是否正确。
 *
 * @en
 * Validate that the minimal service context performs the expected lifecycle transitions.
 */
bool verify_service_context() {
    crynet::core::ServiceContext context{9001, "bootstrap"};

    if (context.handle() != 9001 || context.name() != "bootstrap") {
        std::cerr << "[unit] service context identity mismatch" << std::endl;
        return false;
    }

    if (context.state() != crynet::core::ServiceState::Created) {
        std::cerr << "[unit] service context initial state mismatch" << std::endl;
        return false;
    }

    if (!context.initialize() || context.state() != crynet::core::ServiceState::Initialized) {
        std::cerr << "[unit] service context initialize failed" << std::endl;
        return false;
    }

    if (!context.start() || !context.is_running()) {
        std::cerr << "[unit] service context start failed" << std::endl;
        return false;
    }

    if (crynet::core::service_state_name(context.state()) != "running") {
        std::cerr << "[unit] service context state name mismatch" << std::endl;
        return false;
    }

    context.stop();
    if (context.state() != crynet::core::ServiceState::Stopped) {
        std::cerr << "[unit] service context stop failed" << std::endl;
        return false;
    }

    context.release();
    if (context.state() != crynet::core::ServiceState::Released) {
        std::cerr << "[unit] service context release failed" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证服务句柄分配、注册、按名查询与注销链路是否正确。
 *
 * @en
 * Validate service-handle allocation, registration, name lookup and unregister flow.
 */
bool verify_handle_registry() {
    crynet::core::HandleRegistry registry;
    const auto bootstrap_handle = registry.allocate();
    const auto worker_handle = registry.allocate();

    if (bootstrap_handle == 0 || worker_handle == 0 || bootstrap_handle == worker_handle) {
        std::cerr << "[unit] handle allocation mismatch" << std::endl;
        return false;
    }

    const crynet::core::ServiceContext bootstrap_context{bootstrap_handle, "bootstrap"};
    const crynet::core::ServiceContext worker_context{worker_handle, "worker"};

    if (!registry.register_service(bootstrap_context) || !registry.register_service(worker_context)) {
        std::cerr << "[unit] service registration failed" << std::endl;
        return false;
    }

    if (registry.size() != 2 || !registry.contains(bootstrap_handle)) {
        std::cerr << "[unit] registry size or contain check failed" << std::endl;
        return false;
    }

    const auto resolved_handle = registry.find_handle("bootstrap");
    if (!resolved_handle.has_value() || resolved_handle.value() != bootstrap_handle) {
        std::cerr << "[unit] registry name lookup failed" << std::endl;
        return false;
    }

    const auto resolved_name = registry.find_name(worker_handle);
    if (!resolved_name.has_value() || resolved_name.value() != "worker") {
        std::cerr << "[unit] registry handle lookup failed" << std::endl;
        return false;
    }

    if (registry.register_service(bootstrap_context)) {
        std::cerr << "[unit] duplicate service registration should fail" << std::endl;
        return false;
    }

    if (!registry.unregister(worker_handle) || registry.contains(worker_handle)) {
        std::cerr << "[unit] unregister failed" << std::endl;
        return false;
    }

    if (registry.find_name(worker_handle).has_value() || registry.find_handle("worker").has_value()) {
        std::cerr << "[unit] registry cleanup mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证单服务消息队列的目标路由约束和 FIFO 行为是否正确。
 *
 * @en
 * Validate target-routing constraints and FIFO behavior for the per-service message queue.
 */
bool verify_message_queue() {
    crynet::core::MessageQueue queue{2001};

    if (queue.owner_handle() != 2001 || !queue.empty()) {
        std::cerr << "[unit] message queue initial state mismatch" << std::endl;
        return false;
    }

    const auto first = crynet::core::Message::from_text(
        crynet::core::MessageType::Request,
        1001,
        2001,
        3001,
        "first"
    );
    const auto second = crynet::core::Message::from_text(
        crynet::core::MessageType::Event,
        1002,
        2001,
        3002,
        "second"
    );
    const auto invalid = crynet::core::Message::from_text(
        crynet::core::MessageType::Request,
        1003,
        9999,
        3003,
        "reject"
    );

    if (!queue.enqueue(first) || !queue.enqueue(second)) {
        std::cerr << "[unit] message queue enqueue failed" << std::endl;
        return false;
    }

    if (queue.enqueue(invalid)) {
        std::cerr << "[unit] message queue should reject mismatched target handle" << std::endl;
        return false;
    }

    if (queue.size() != 2) {
        std::cerr << "[unit] message queue size mismatch" << std::endl;
        return false;
    }

    const auto first_out = queue.dequeue();
    const auto second_out = queue.dequeue();
    const auto empty_out = queue.dequeue();

    if (!first_out.has_value() || first_out->text_payload() != "first") {
        std::cerr << "[unit] message queue first dequeue mismatch" << std::endl;
        return false;
    }

    if (!second_out.has_value() || second_out->text_payload() != "second") {
        std::cerr << "[unit] message queue second dequeue mismatch" << std::endl;
        return false;
    }

    if (empty_out.has_value() || !queue.empty()) {
        std::cerr << "[unit] message queue empty dequeue mismatch" << std::endl;
        return false;
    }

    crynet::core::MessageQueue overload_queue{2002};
    for (std::size_t index = 0; index < 1025; ++index) {
        if (!overload_queue.enqueue(crynet::core::Message::from_text(
                crynet::core::MessageType::Event,
                1001,
                2002,
                static_cast<crynet::core::SessionId>(index + 1),
                "overload"
            ))) {
            std::cerr << "[unit] message queue overload enqueue failed" << std::endl;
            return false;
        }
    }

    const auto overload_size = overload_queue.take_overload();
    if (!overload_size.has_value() || overload_size.value() != 1025 || overload_queue.peak_size() != 1025) {
        std::cerr << "[unit] message queue overload tracking mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证全局就绪队列的 FIFO 顺序与去重行为是否正确。
 *
 * @en
 * Validate FIFO ordering and duplicate suppression for the global ready queue.
 */
bool verify_ready_queue() {
    crynet::core::ReadyQueue queue;

    if (!queue.empty() || queue.size() != 0) {
        std::cerr << "[unit] ready queue initial state mismatch" << std::endl;
        return false;
    }

    if (!queue.push(1001) || !queue.push(1002)) {
        std::cerr << "[unit] ready queue push failed" << std::endl;
        return false;
    }

    if (queue.push(1001) || queue.push(0)) {
        std::cerr << "[unit] ready queue should reject duplicate or zero handles" << std::endl;
        return false;
    }

    const auto first = queue.pop();
    const auto second = queue.pop();
    const auto empty = queue.pop();

    if (!first.has_value() || first.value() != 1001) {
        std::cerr << "[unit] ready queue first pop mismatch" << std::endl;
        return false;
    }

    if (!second.has_value() || second.value() != 1002) {
        std::cerr << "[unit] ready queue second pop mismatch" << std::endl;
        return false;
    }

    if (empty.has_value() || !queue.empty()) {
        std::cerr << "[unit] ready queue empty pop mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证 worker 调度器的就绪分发、执行中跟踪和完成后重入语义是否正确。
 *
 * @en
 * Validate runnable dispatch, in-flight tracking and post-completion rescheduling semantics for the worker scheduler.
 */
bool verify_worker_scheduler() {
    crynet::core::WorkerScheduler scheduler;

    if (!scheduler.mark_ready(1001) || !scheduler.mark_ready(1002)) {
        std::cerr << "[unit] worker scheduler mark_ready failed" << std::endl;
        return false;
    }

    if (scheduler.mark_ready(1001) || scheduler.pending_count() != 2) {
        std::cerr << "[unit] worker scheduler duplicate ready mismatch" << std::endl;
        return false;
    }

    const auto first = scheduler.acquire_next();
    if (!first.has_value() || first.value() != 1001 || !scheduler.is_in_flight(1001)) {
        std::cerr << "[unit] worker scheduler first acquire mismatch" << std::endl;
        return false;
    }

    if (scheduler.mark_ready(1001)) {
        std::cerr << "[unit] worker scheduler should not reschedule in-flight service" << std::endl;
        return false;
    }

    const auto second = scheduler.acquire_next();
    if (!second.has_value() || second.value() != 1002 || scheduler.in_flight_count() != 2) {
        std::cerr << "[unit] worker scheduler second acquire mismatch" << std::endl;
        return false;
    }

    if (!scheduler.complete(1001) || scheduler.is_in_flight(1001)) {
        std::cerr << "[unit] worker scheduler complete mismatch" << std::endl;
        return false;
    }

    if (!scheduler.mark_ready(1001)) {
        std::cerr << "[unit] worker scheduler should allow reschedule after completion" << std::endl;
        return false;
    }

    const auto third = scheduler.acquire_next();
    if (!third.has_value() || third.value() != 1001) {
        std::cerr << "[unit] worker scheduler reschedule acquire mismatch" << std::endl;
        return false;
    }

    if (!scheduler.complete(1002) || !scheduler.complete(1001) || scheduler.in_flight_count() != 0) {
        std::cerr << "[unit] worker scheduler final completion mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证超时队列的调度、取消和到期投递行为是否正确。
 *
 * @en
 * Validate scheduling, cancellation and expiration delivery behavior for the timeout queue.
 */
bool verify_timer_queue() {
    crynet::core::TimerQueue timer_queue;

    const auto start = std::chrono::steady_clock::now();

    const auto fired_id = timer_queue.schedule_after(2001, 3001, std::chrono::milliseconds{1}, "fire");
    const auto cancelled_id = timer_queue.schedule_after(2002, 3002, std::chrono::milliseconds{5}, "cancel");
    const auto near_id = timer_queue.schedule_after(2003, 3003, std::chrono::milliseconds{300}, "near");
    const auto cascaded_id = timer_queue.schedule_after(2004, 3004, std::chrono::milliseconds{17000}, "cascaded");

    if (fired_id == 0 || cancelled_id == 0 || near_id == 0 || cascaded_id == 0 || fired_id == cancelled_id) {
        std::cerr << "[unit] timer queue id allocation mismatch" << std::endl;
        return false;
    }

    if (!timer_queue.cancel(cancelled_id) || timer_queue.cancel(cancelled_id)) {
        std::cerr << "[unit] timer queue cancel mismatch" << std::endl;
        return false;
    }

    const auto immediate = timer_queue.poll_expired(start);
    if (!immediate.empty()) {
        std::cerr << "[unit] timer queue should not fire before deadline" << std::endl;
        return false;
    }

    const auto expired = timer_queue.poll_expired(start + std::chrono::milliseconds{10});
    if (expired.size() != 1) {
        std::cerr << "[unit] timer queue expiration count mismatch" << std::endl;
        return false;
    }

    const auto& message = expired.front();
    if (message.type() != crynet::core::MessageType::Timer || message.target_handle() != 2001 ||
        message.session_id() != 3001 || message.text_payload() != "fire") {
        std::cerr << "[unit] timer queue expiration payload mismatch" << std::endl;
        return false;
    }

    const auto before_near = timer_queue.poll_expired(start + std::chrono::milliseconds{299});
    if (!before_near.empty()) {
        std::cerr << "[unit] timer queue should keep near-level task pending" << std::endl;
        return false;
    }

    const auto near_expired = timer_queue.poll_expired(start + std::chrono::milliseconds{300});
    if (near_expired.size() != 1 || near_expired.front().target_handle() != 2003 ||
        near_expired.front().text_payload() != "near") {
        std::cerr << "[unit] timer queue near-level cascade mismatch" << std::endl;
        return false;
    }

    const auto before_cascaded = timer_queue.poll_expired(start + std::chrono::milliseconds{16999});
    if (!before_cascaded.empty()) {
        std::cerr << "[unit] timer queue should keep upper-level task pending" << std::endl;
        return false;
    }

    const auto cascaded_expired = timer_queue.poll_expired(start + std::chrono::milliseconds{17000});
    if (cascaded_expired.size() != 1 || cascaded_expired.front().target_handle() != 2004 ||
        cascaded_expired.front().session_id() != 3004 || cascaded_expired.front().text_payload() != "cascaded") {
        std::cerr << "[unit] timer queue upper-level cascade mismatch" << std::endl;
        return false;
    }

    if (timer_queue.size() != 0) {
        std::cerr << "[unit] timer queue cleanup mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证会话管理器的会话分配、响应匹配和完成清理行为是否正确。
 *
 * @en
 * Validate session allocation, response matching, and completion cleanup behavior for the session manager.
 */
bool verify_session_manager() {
    crynet::core::SessionManager session_manager;

    const auto first = session_manager.begin(1001, 2001);
    const auto second = session_manager.begin(1002, 2002);

    if (first == 0 || second == 0 || first == second || session_manager.size() != 2) {
        std::cerr << "[unit] session manager allocation mismatch" << std::endl;
        return false;
    }

    const auto requester = session_manager.lookup_requester(first, 2001);
    if (!requester.has_value() || requester.value() != 1001) {
        std::cerr << "[unit] session manager lookup mismatch" << std::endl;
        return false;
    }

    if (session_manager.lookup_requester(first, 9999).has_value()) {
        std::cerr << "[unit] session manager should reject mismatched responder" << std::endl;
        return false;
    }

    if (!session_manager.complete(first, 2001) || session_manager.complete(first, 2001) ||
        session_manager.contains(first)) {
        std::cerr << "[unit] session manager completion mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证最小 Actor 系统是否能够完成服务注册、消息执行和超时消息注入。
 *
 * @en
 * Validate that the minimal actor system can complete service registration,
 * message execution, and timeout-message injection.
 */
bool verify_actor_system() {
    crynet::core::ActorSystem runtime;
    std::vector<std::string> observed_messages;

    const auto caller_handle = runtime.register_service(
        "caller",
        [&observed_messages](crynet::core::ServiceContext& context, const crynet::core::Message& message) {
            observed_messages.push_back(
                std::string{context.name()} + ":" + std::string{message.text_payload()}
            );
        }
    );
    const auto service_handle = runtime.register_service(
        "echo",
        [&runtime, &observed_messages](crynet::core::ServiceContext& context, const crynet::core::Message& message) {
            observed_messages.push_back(
                std::string{context.name()} + ":" + std::string{message.text_payload()}
            );

            if (message.type() == crynet::core::MessageType::Request) {
                static_cast<void>(runtime.respond(context.handle(), message.session_id(), "world"));
            }
        }
    );

    if (!caller_handle.has_value() || !service_handle.has_value() || runtime.service_count() != 2) {
        std::cerr << "[unit] actor system service registration mismatch" << std::endl;
        return false;
    }

    const auto session_id = runtime.call(caller_handle.value(), service_handle.value(), "hello");
    if (!session_id.has_value() || runtime.pending_sessions() != 1) {
        std::cerr << "[unit] actor system call failed" << std::endl;
        return false;
    }

    if (runtime.pending_messages(service_handle.value()) != 1 || !runtime.dispatch_once()) {
        std::cerr << "[unit] actor system request dispatch failed" << std::endl;
        return false;
    }

    if (observed_messages.size() != 1 || observed_messages.front() != "echo:hello" ||
        runtime.pending_sessions() != 0 || runtime.pending_messages(caller_handle.value()) != 1) {
        std::cerr << "[unit] actor system request-response enqueue mismatch" << std::endl;
        return false;
    }

    if (!runtime.dispatch_once()) {
        std::cerr << "[unit] actor system response dispatch failed" << std::endl;
        return false;
    }

    if (observed_messages.size() != 2 || observed_messages.back() != "caller:world") {
        std::cerr << "[unit] actor system response execution mismatch" << std::endl;
        return false;
    }

    const auto timer_id = runtime.schedule_timeout(
        caller_handle.value(),
        4001,
        std::chrono::milliseconds{1},
        "wake"
    );

    if (timer_id == 0 || runtime.pump_timers(std::chrono::steady_clock::now() + std::chrono::milliseconds{10}) != 1) {
        std::cerr << "[unit] actor system timer pump mismatch" << std::endl;
        return false;
    }

    if (!runtime.dispatch_once()) {
        std::cerr << "[unit] actor system timer dispatch failed" << std::endl;
        return false;
    }

    if (observed_messages.size() != 3 || observed_messages.back() != "caller:wake") {
        std::cerr << "[unit] actor system timer execution mismatch" << std::endl;
        return false;
    }

    if (!runtime.send(crynet::core::Message::from_text(
            crynet::core::MessageType::Event,
            0,
            caller_handle.value(),
            0,
            "batch-1"
        )) ||
        !runtime.send(crynet::core::Message::from_text(
            crynet::core::MessageType::Event,
            0,
            caller_handle.value(),
            0,
            "batch-2"
        )) ||
        !runtime.send(crynet::core::Message::from_text(
            crynet::core::MessageType::Event,
            0,
            caller_handle.value(),
            0,
            "batch-3"
        ))) {
        std::cerr << "[unit] actor system batch enqueue failed" << std::endl;
        return false;
    }

    if (runtime.dispatch_batch(2) != 2 || runtime.pending_messages(caller_handle.value()) != 1) {
        std::cerr << "[unit] actor system batch dispatch mismatch" << std::endl;
        return false;
    }

    if (runtime.run_until_idle(4, 2) != 1 || observed_messages.back() != "caller:batch-3") {
        std::cerr << "[unit] actor system run_until_idle mismatch" << std::endl;
        return false;
    }

    if (!runtime.stop_service(caller_handle.value())) {
        std::cerr << "[unit] actor system stop service failed" << std::endl;
        return false;
    }

    const auto stopped_state = runtime.service_state(caller_handle.value());
    if (!stopped_state.has_value() || stopped_state.value() != crynet::core::ServiceState::Stopped) {
        std::cerr << "[unit] actor system stop state mismatch" << std::endl;
        return false;
    }

    if (runtime.send(crynet::core::Message::from_text(
            crynet::core::MessageType::Event,
            service_handle.value(),
            caller_handle.value(),
            0,
            "after-stop"
        ))) {
        std::cerr << "[unit] actor system should reject messages to stopped services" << std::endl;
        return false;
    }

    const auto released_timer = runtime.schedule_timeout(
        service_handle.value(),
        5001,
        std::chrono::milliseconds{1000},
        "release"
    );
    if (released_timer == 0) {
        std::cerr << "[unit] actor system release timer allocation failed" << std::endl;
        return false;
    }

    if (!runtime.release_service(service_handle.value()) || runtime.find_service_handle("echo").has_value() ||
        runtime.service_state(service_handle.value()).has_value()) {
        std::cerr << "[unit] actor system release service mismatch" << std::endl;
        return false;
    }

    if (runtime.pending_sessions() != 0 || runtime.service_count() != 1) {
        std::cerr << "[unit] actor system release cleanup mismatch" << std::endl;
        return false;
    }

    if (runtime.shutdown() != 1 || runtime.service_count() != 0) {
        std::cerr << "[unit] actor system shutdown mismatch" << std::endl;
        return false;
    }

    const auto metrics = runtime.metrics();
    if (metrics.registered_services != 0 || metrics.pending_sessions != 0 || metrics.queued_messages != 0 ||
        metrics.enqueued_messages != 6 || metrics.dispatched_messages != 6 || metrics.timer_messages != 1 ||
        metrics.dropped_messages != 1 || metrics.queue_overload_events != 0 || metrics.peak_queue_depth != 3) {
        std::cerr << "[unit] actor system metrics mismatch"
                  << " registered=" << metrics.registered_services
                  << " pending_sessions=" << metrics.pending_sessions
                  << " queued=" << metrics.queued_messages
                  << " enqueued=" << metrics.enqueued_messages
                  << " dispatched=" << metrics.dispatched_messages
                  << " timers=" << metrics.timer_messages
                  << " dropped=" << metrics.dropped_messages
                  << " overloads=" << metrics.queue_overload_events
                  << " peak_queue_depth=" << metrics.peak_queue_depth
                  << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证 worker 循环是否能统一驱动定时器注入、批量分发以及停止控制。
 *
 * @en
 * Validate that the worker loop can drive timer injection, batched dispatch,
 * and stop control through one execution entry point.
 */
bool verify_worker_loop() {
    crynet::core::ActorSystem system;
    std::vector<std::string> observed_messages;

    const auto service_handle = system.register_service(
        "worker",
        [&observed_messages](crynet::core::ServiceContext& context, const crynet::core::Message& message) {
            observed_messages.push_back(
                std::string{context.name()} + ":" + std::string{message.text_payload()}
            );
        }
    );
    if (!service_handle.has_value()) {
        std::cerr << "[unit] worker loop service registration failed" << std::endl;
        return false;
    }

    crynet::core::WorkerLoop worker_loop{system};
    const auto start = std::chrono::steady_clock::now();

    if (!system.send(crynet::core::Message::from_text(
            crynet::core::MessageType::Event,
            0,
            service_handle.value(),
            0,
            "immediate"
        ))) {
        std::cerr << "[unit] worker loop immediate enqueue failed" << std::endl;
        return false;
    }

    const auto delayed_timer = system.schedule_timeout(
        service_handle.value(),
        4101,
        std::chrono::milliseconds{300},
        "delayed"
    );
    if (delayed_timer == 0) {
        std::cerr << "[unit] worker loop delayed timer allocation failed" << std::endl;
        return false;
    }

    const auto first_turn = worker_loop.run_once(start, 2);
    if (first_turn.injected_timer_messages != 0 || first_turn.dispatched_messages != 1 ||
        observed_messages.size() != 1 || observed_messages.front() != "worker:immediate") {
        std::cerr << "[unit] worker loop first turn mismatch" << std::endl;
        return false;
    }

    const auto idle_turn = worker_loop.run_once(start + std::chrono::milliseconds{299}, 2);
    if (!idle_turn.idle()) {
        std::cerr << "[unit] worker loop should remain idle before timeout" << std::endl;
        return false;
    }

    const auto timeout_turn = worker_loop.run_once(start + std::chrono::milliseconds{300}, 2);
    if (timeout_turn.injected_timer_messages != 1 || timeout_turn.dispatched_messages != 1 ||
        observed_messages.size() != 2 || observed_messages.back() != "worker:delayed") {
        std::cerr << "[unit] worker loop timeout turn mismatch" << std::endl;
        return false;
    }

    if (!system.send(crynet::core::Message::from_text(
            crynet::core::MessageType::Event,
            0,
            service_handle.value(),
            0,
            "after-stop"
        ))) {
        std::cerr << "[unit] worker loop stop test enqueue failed" << std::endl;
        return false;
    }

    worker_loop.request_stop();
    if (!worker_loop.stop_requested() || worker_loop.run_until_idle(4, 2) != 0 ||
        system.pending_messages(service_handle.value()) != 1) {
        std::cerr << "[unit] worker loop stop request mismatch" << std::endl;
        return false;
    }

    worker_loop.reset_stop();
    if (worker_loop.stop_requested() || worker_loop.run_until_idle(4, 2) != 1 ||
        system.pending_messages(service_handle.value()) != 0 || observed_messages.back() != "worker:after-stop") {
        std::cerr << "[unit] worker loop reset mismatch" << std::endl;
        return false;
    }

    const auto metrics = system.metrics();
    if (metrics.timer_messages != 1 || metrics.dispatched_messages != 3 || metrics.queued_messages != 0) {
        std::cerr << "[unit] worker loop metrics mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证 worker monitor 的 trigger/check/clear 语义是否接近 skynet monitor。
 *
 * @en
 * Validate that worker-monitor trigger/check/clear semantics align with skynet monitor behavior.
 */
bool verify_worker_monitor() {
    crynet::core::WorkerMonitor monitor{2};
    auto& logger = crynet::core::Logger::default_logger();
    std::vector<std::string> captured_lines;
    std::vector<crynet::core::WorkerMonitorAlert> alerts;

    logger.set_level(crynet::core::LogLevel::Info);
    logger.set_sink(
        [&captured_lines](const crynet::core::LogRecord& record, std::string_view formatted_line) {
            static_cast<void>(record);
            captured_lines.emplace_back(formatted_line);
        }
    );
    monitor.set_alert_sink(
        [&alerts](const crynet::core::WorkerMonitorAlert& alert) {
            alerts.push_back(alert);
        }
    );

    monitor.mark_sleeping(0);
    monitor.mark_sleeping(1);
    if (monitor.snapshot().sleeping_workers != 2 || !monitor.should_wake(0)) {
        std::cerr << "[unit] worker monitor initial sleep mismatch" << std::endl;
        return false;
    }

    monitor.trigger(0, crynet::core::DispatchTrace{
        .message_type = crynet::core::MessageType::Request,
        .source_handle = 1001,
        .target_handle = 2001,
        .session_id = 3001,
    });

    const auto active_route = monitor.active_route(0);
    if (!active_route.has_value() || active_route->source_handle != 1001 || active_route->target_handle != 2001 ||
        active_route->session_id != 3001 || active_route->message_type != crynet::core::MessageType::Request) {
        std::cerr << "[unit] worker monitor trigger route mismatch" << std::endl;
        return false;
    }

    if (monitor.check_stalled(0).has_value()) {
        std::cerr << "[unit] worker monitor should not report stall on first check" << std::endl;
        return false;
    }

    const auto stalled_route = monitor.check_stalled(0);
    if (!stalled_route.has_value() || stalled_route->source_handle != 1001 || stalled_route->target_handle != 2001) {
        logger.reset();
        std::cerr << "[unit] worker monitor stall detection mismatch" << std::endl;
        return false;
    }

    if (alerts.size() != 1 || alerts.front().worker_id != 0 || alerts.front().route.target_handle != 2001 ||
        alerts.front().message.find("worker 0 may be stalled") == std::string::npos) {
        logger.reset();
        std::cerr << "[unit] worker monitor alert callback mismatch" << std::endl;
        return false;
    }

    if (captured_lines.size() != 1 || captured_lines.front().find("[error] [monitor] worker 0 may be stalled") == std::string::npos) {
        logger.reset();
        std::cerr << "[unit] worker monitor logger alert mismatch" << std::endl;
        return false;
    }

    monitor.clear(0);
    if (monitor.active_route(0).has_value() || monitor.check_stalled(0).has_value()) {
        logger.reset();
        std::cerr << "[unit] worker monitor clear mismatch" << std::endl;
        return false;
    }

    if (monitor.snapshot().stall_events != 1) {
        logger.reset();
        std::cerr << "[unit] worker monitor stall event mismatch" << std::endl;
        return false;
    }

    logger.reset();

    return true;
}

/**
 * @cn
 * 验证 worker monitor 与 runner 是否能维护 sleep/wake/quit 语义。
 *
 * @en
 * Validate that the worker monitor and runner preserve sleep, wake, and quit semantics.
 */
bool verify_worker_runner() {
    crynet::core::ActorSystem system;
    std::vector<std::string> observed_messages;

    const auto service_handle = system.register_service(
        "runner",
        [&observed_messages](crynet::core::ServiceContext& context, const crynet::core::Message& message) {
            observed_messages.push_back(
                std::string{context.name()} + ":" + std::string{message.text_payload()}
            );
        }
    );
    if (!service_handle.has_value()) {
        std::cerr << "[unit] worker runner service registration failed" << std::endl;
        return false;
    }

    crynet::core::WorkerMonitor monitor{1};
    crynet::core::WorkerRunner runner{system, 0};
    const auto start = std::chrono::steady_clock::now();

    if (runner.plugin_count() != 0) {
        std::cerr << "[unit] worker runner initial plugin count mismatch" << std::endl;
        return false;
    }

    const auto initial_turn = runner.run_once(start, 1);
    if (!initial_turn.idle() || system.pending_messages(service_handle.value()) != 0) {
        std::cerr << "[unit] worker runner initial sleep mismatch" << std::endl;
        return false;
    }

    runner.add_plugin(monitor);
    if (runner.plugin_count() != 1) {
        std::cerr << "[unit] worker runner plugin attach mismatch" << std::endl;
        return false;
    }

    const auto plugin_idle_turn = runner.run_once(start, 1);
    const auto initial_snapshot = monitor.snapshot();
    if (!plugin_idle_turn.idle() || initial_snapshot.sleeping_workers != 1 || initial_snapshot.wake_events != 0 ||
        initial_snapshot.busy_turns != 0 || !monitor.should_wake(0)) {
        std::cerr << "[unit] worker runner plugin idle mismatch" << std::endl;
        return false;
    }

    if (!system.send(crynet::core::Message::from_text(
            crynet::core::MessageType::Event,
            0,
            service_handle.value(),
            0,
            "wake"
        ))) {
        std::cerr << "[unit] worker runner enqueue failed" << std::endl;
        return false;
    }

    const auto active_turn = runner.run_once(start + std::chrono::milliseconds{1}, 1);
    const auto active_snapshot = monitor.snapshot();
    if (active_turn.dispatched_messages != 1 || observed_messages.size() != 1 || observed_messages.front() != "runner:wake" ||
        active_snapshot.sleeping_workers != 0 || active_snapshot.wake_events != 1 || active_snapshot.busy_turns != 1 ||
        active_snapshot.stall_events != 0 || monitor.active_route(0).has_value() || monitor.check_stalled(0).has_value()) {
        std::cerr << "[unit] worker runner wake mismatch" << std::endl;
        return false;
    }

    if (!system.send(crynet::core::Message::from_text(
            crynet::core::MessageType::Event,
            0,
            service_handle.value(),
            0,
            "quit"
        ))) {
        std::cerr << "[unit] worker runner quit enqueue failed" << std::endl;
        return false;
    }

    monitor.request_quit();
    if (!monitor.quit_requested() || runner.run_until_idle(4, 1) != 0 || system.pending_messages(service_handle.value()) != 1) {
        std::cerr << "[unit] worker runner quit mismatch" << std::endl;
        return false;
    }

    const auto quit_snapshot = monitor.snapshot();
    if (!quit_snapshot.quit_requested || quit_snapshot.busy_turns != 1 || quit_snapshot.wake_events != 1) {
        std::cerr << "[unit] worker runner quit snapshot mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证不挂载任何插件时 worker runner 仍能走零插件快路径完成消息分发。
 *
 * @en
 * Validate that the worker runner can still complete message dispatch through the zero-plugin fast path.
 */
bool verify_worker_runner_without_plugins() {
    crynet::core::ActorSystem system;
    std::vector<std::string> observed_messages;

    const auto service_handle = system.register_service(
        "plain-runner",
        [&observed_messages](crynet::core::ServiceContext& context, const crynet::core::Message& message) {
            observed_messages.push_back(
                std::string{context.name()} + ":" + std::string{message.text_payload()}
            );
        }
    );
    if (!service_handle.has_value()) {
        std::cerr << "[unit] worker runner no-plugin service registration failed" << std::endl;
        return false;
    }

    crynet::core::WorkerRunner runner{system, 0};
    if (runner.plugin_count() != 0) {
        std::cerr << "[unit] worker runner no-plugin count mismatch" << std::endl;
        return false;
    }

    if (!system.send(crynet::core::Message::from_text(
            crynet::core::MessageType::Event,
            0,
            service_handle.value(),
            0,
            "plain"
        ))) {
        std::cerr << "[unit] worker runner no-plugin enqueue failed" << std::endl;
        return false;
    }

    if (runner.run_until_idle(2, 1) != 1 || observed_messages.size() != 1 || observed_messages.front() != "plain-runner:plain") {
        std::cerr << "[unit] worker runner no-plugin dispatch mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证 monitor runner 是否能按轮次扫描 worker monitor 并触发 stall 告警。
 *
 * @en
 * Validate that the monitor runner can scan the worker monitor per pass and trigger stall alerts.
 */
bool verify_monitor_runner() {
    crynet::core::WorkerMonitor monitor{2};
    crynet::monitor::MonitorRunner runner{monitor};
    auto& logger = crynet::core::Logger::default_logger();
    std::vector<std::string> captured_lines;

    logger.set_level(crynet::core::LogLevel::Info);
    logger.set_sink(
        [&captured_lines](const crynet::core::LogRecord& record, std::string_view formatted_line) {
            static_cast<void>(record);
            captured_lines.emplace_back(formatted_line);
        }
    );

    monitor.trigger(0, crynet::core::DispatchTrace{
        .message_type = crynet::core::MessageType::Event,
        .source_handle = 7001,
        .target_handle = 8001,
        .session_id = 9001,
    });

    const auto first_pass = runner.check_once();
    if (first_pass.scanned_workers != 2 || first_pass.detected_stalls != 0) {
        logger.reset();
        std::cerr << "[unit] monitor runner first pass mismatch" << std::endl;
        return false;
    }

    const auto second_pass = runner.check_once();
    if (second_pass.scanned_workers != 2 || second_pass.detected_stalls != 1 ||
        captured_lines.size() != 1 || captured_lines.front().find("[error] [monitor] worker 0 may be stalled") == std::string::npos) {
        logger.reset();
        std::cerr << "[unit] monitor runner second pass mismatch" << std::endl;
        return false;
    }

    monitor.request_quit();
    if (runner.run_checks(4) != 0) {
        logger.reset();
        std::cerr << "[unit] monitor runner quit mismatch" << std::endl;
        return false;
    }

    logger.reset();
    return true;
}

/**
 * @cn
 * 验证 bootstrap 文本配置是否能够被正确解析。
 *
 * @en
 * Validate that the bootstrap text configuration can be parsed correctly.
 */
bool verify_bootstrap_config() {
    const auto config = crynet::bootstrap::BootstrapConfig::from_text(
        "service bootstrap ready\n"
        "service echo hello\n"
        "timeout_ms 25\n"
        "monitor.enabled true\n"
        "monitor.logger_alert off\n"
        "monitor.check_cycles 3\n"
    );

    if (!config.has_value()) {
        std::cerr << "[unit] bootstrap config parse failed" << std::endl;
        return false;
    }

    if (config->services().size() != 2 || config->services().front().name != "bootstrap" ||
        config->services().front().boot_payload != "ready" || config->timeout() != std::chrono::milliseconds{25} ||
        !config->monitor().enabled || config->monitor().logger_alert_enabled || config->monitor().check_cycles != 3) {
        std::cerr << "[unit] bootstrap config content mismatch" << std::endl;
        return false;
    }

    if (crynet::bootstrap::BootstrapConfig::from_text("timeout_ms nope\n").has_value()) {
        std::cerr << "[unit] bootstrap config should reject invalid timeout" << std::endl;
        return false;
    }

    if (crynet::bootstrap::BootstrapConfig::from_text("service bootstrap ready\nmonitor.enabled maybe\n").has_value()) {
        std::cerr << "[unit] bootstrap config should reject invalid monitor bool" << std::endl;
        return false;
    }

    const auto file_path = std::filesystem::temp_directory_path() / "crynet-bootstrap-config-test.cfg";
    {
        std::ofstream output{file_path};
        output << "service bootstrap from-file\n";
        output << "timeout_ms 30\n";
        output << "monitor.enabled on\n";
        output << "monitor.check_cycles 2\n";
    }

    const auto file_config = crynet::bootstrap::BootstrapConfig::from_file(file_path);
    std::filesystem::remove(file_path);
    if (!file_config.has_value() || file_config->services().size() != 1 ||
        file_config->services().front().boot_payload != "from-file" ||
        file_config->timeout() != std::chrono::milliseconds{30} || !file_config->monitor().enabled ||
        file_config->monitor().check_cycles != 2) {
        std::cerr << "[unit] bootstrap config file loading mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证运行时 bootstrap 是否能够注册服务、按名查询并执行初始消息。
 *
 * @en
 * Validate that runtime bootstrap can register services, resolve them by name, and execute initial messages.
 */
bool verify_bootstrapper() {
    const auto config = crynet::bootstrap::BootstrapConfig::from_text(
        "service bootstrap boot\n"
        "service worker wake\n"
    );
    if (!config.has_value()) {
        std::cerr << "[unit] bootstrapper config parse failed" << std::endl;
        return false;
    }

    crynet::bootstrap::Bootstrapper bootstrapper;
    std::vector<std::string> observed_messages;

    bootstrapper.bind_handler(
        "bootstrap",
        [&observed_messages](crynet::core::ServiceContext& context, const crynet::core::Message& message) {
            observed_messages.push_back(std::string{context.name()} + ":" + std::string{message.text_payload()});
        }
    );
    bootstrapper.bind_handler(
        "worker",
        [&observed_messages](crynet::core::ServiceContext& context, const crynet::core::Message& message) {
            observed_messages.push_back(std::string{context.name()} + ":" + std::string{message.text_payload()});
        }
    );

    if (!bootstrapper.start(config.value())) {
        std::cerr << "[unit] bootstrapper start failed" << std::endl;
        return false;
    }

    const auto bootstrap_handle = bootstrapper.find_service_handle("bootstrap");
    const auto worker_handle = bootstrapper.find_service_handle("worker");
    if (!bootstrap_handle.has_value() || !worker_handle.has_value()) {
        std::cerr << "[unit] bootstrapper service lookup failed" << std::endl;
        return false;
    }

    if (bootstrapper.system().service_count() != 2 || bootstrapper.system().pending_messages(bootstrap_handle.value()) != 1 ||
        bootstrapper.system().pending_messages(worker_handle.value()) != 1) {
        std::cerr << "[unit] bootstrapper pending message mismatch" << std::endl;
        return false;
    }

    if (bootstrapper.dispatch_until_idle() != 2) {
        std::cerr << "[unit] bootstrapper dispatch count mismatch" << std::endl;
        return false;
    }

    if (observed_messages.size() != 2 || observed_messages.front() != "bootstrap:boot" ||
        observed_messages.back() != "worker:wake") {
        std::cerr << "[unit] bootstrapper execution mismatch" << std::endl;
        return false;
    }

    const auto metrics = bootstrapper.system().metrics();
    if (metrics.registered_services != 2 || metrics.queued_messages != 0 || metrics.dispatched_messages != 2) {
        std::cerr << "[unit] bootstrapper metrics mismatch" << std::endl;
        return false;
    }

    return true;
}

/**
 * @cn
 * 验证 logger 是否支持级别过滤、自定义 sink 与格式化输出。
 *
 * @en
 * Validate that the logger supports level filtering, custom sinks, and formatted output.
 */
bool verify_logger() {
    auto& logger = crynet::core::Logger::default_logger();
    std::vector<std::string> captured_lines;
    std::vector<crynet::core::LogRecord> captured_records;

    logger.set_level(crynet::core::LogLevel::Info);
    logger.set_sink(
        [&captured_lines, &captured_records](const crynet::core::LogRecord& record, std::string_view formatted_line) {
            captured_records.push_back(record);
            captured_lines.emplace_back(formatted_line);
        }
    );

    logger.log(crynet::core::LogLevel::Debug, "logger", "skip-me");
    logger.log(crynet::core::LogLevel::Info, "startup", "ready");
    logger.log(crynet::core::LogLevel::Error, "actor", "failed");

    logger.reset();

    if (captured_records.size() != 2 || captured_lines.size() != 2) {
        std::cerr << "[unit] logger capture count mismatch" << std::endl;
        return false;
    }

    if (captured_records.front().level != crynet::core::LogLevel::Info ||
        captured_records.front().component != "startup" ||
        captured_records.front().message != "ready") {
        std::cerr << "[unit] logger record payload mismatch" << std::endl;
        return false;
    }

    if (captured_lines.front() != "[info] [startup] ready" ||
        captured_lines.back() != "[error] [actor] failed") {
        std::cerr << "[unit] logger formatted line mismatch" << std::endl;
        return false;
    }

    if (crynet::core::log_level_name(crynet::core::LogLevel::Warn) != "warn") {
        std::cerr << "[unit] logger level name mismatch" << std::endl;
        return false;
    }

    return true;
}

}  // namespace

int SmokeSuite::run() noexcept {
    crynet::startup::StartupApp startup_app;
    if (!startup_app.initialize()) {
        std::cerr << "[unit] startup initialization failed" << std::endl;
        return 1;
    }

    if (!startup_app.is_initialized()) {
        std::cerr << "[unit] startup initialized state mismatch" << std::endl;
        return 1;
    }

    if (!startup_app.bootstrap_ready()) {
        std::cerr << "[unit] startup bootstrap readiness mismatch" << std::endl;
        return 1;
    }

    if (!verify_message_model()) {
        return 1;
    }

    if (!verify_service_context()) {
        return 1;
    }

    if (!verify_handle_registry()) {
        return 1;
    }

    if (!verify_message_queue()) {
        return 1;
    }

    if (!verify_ready_queue()) {
        return 1;
    }

    if (!verify_worker_scheduler()) {
        return 1;
    }

    if (!verify_timer_queue()) {
        return 1;
    }

    if (!verify_session_manager()) {
        return 1;
    }

    if (!verify_actor_system()) {
        return 1;
    }

    if (!verify_worker_loop()) {
        return 1;
    }

    if (!verify_worker_monitor()) {
        return 1;
    }

    if (!verify_worker_runner()) {
        return 1;
    }

    if (!verify_worker_runner_without_plugins()) {
        return 1;
    }

    if (!verify_monitor_runner()) {
        return 1;
    }

    if (!verify_bootstrap_config()) {
        return 1;
    }

    if (!verify_bootstrapper()) {
        return 1;
    }

    if (!verify_logger()) {
        return 1;
    }

    startup_app.shutdown();
    std::cout << "[unit] smoke suite passed" << std::endl;
    return 0;
}

}  // namespace crynet::tests::unit

int main() {
    return crynet::tests::unit::SmokeSuite::run();
}
