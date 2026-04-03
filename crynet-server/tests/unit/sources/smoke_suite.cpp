#include "tests/unit/includes/smoke_suite.h"

#include "core/actor/includes/message.h"
#include "core/actor/includes/actor_system.h"
#include "core/bootstrap/includes/bootstrap_config.h"
#include "core/bootstrap/includes/bootstrapper.h"
#include "core/actor/includes/handle_registry.h"
#include "core/actor/includes/message_queue.h"
#include "core/actor/includes/session_manager.h"
#include "core/actor/includes/service_context.h"
#include "core/scheduler/includes/ready_queue.h"
#include "core/scheduler/includes/worker_scheduler.h"
#include "core/scheduler/timer/includes/timer_queue.h"
#include "startup/app/includes/startup_app.h"

#include <chrono>
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

    const auto fired_id = timer_queue.schedule_after(2001, 3001, std::chrono::milliseconds{1}, "fire");
    const auto cancelled_id = timer_queue.schedule_after(2002, 3002, std::chrono::milliseconds{5}, "cancel");

    if (fired_id == 0 || cancelled_id == 0 || fired_id == cancelled_id) {
        std::cerr << "[unit] timer queue id allocation mismatch" << std::endl;
        return false;
    }

    if (!timer_queue.cancel(cancelled_id) || timer_queue.cancel(cancelled_id)) {
        std::cerr << "[unit] timer queue cancel mismatch" << std::endl;
        return false;
    }

    const auto immediate = timer_queue.poll_expired(std::chrono::steady_clock::now());
    if (!immediate.empty()) {
        std::cerr << "[unit] timer queue should not fire before deadline" << std::endl;
        return false;
    }

    const auto expired = timer_queue.poll_expired(std::chrono::steady_clock::now() + std::chrono::milliseconds{10});
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
    );

    if (!config.has_value()) {
        std::cerr << "[unit] bootstrap config parse failed" << std::endl;
        return false;
    }

    if (config->services().size() != 2 || config->services().front().name != "bootstrap" ||
        config->services().front().boot_payload != "ready" || config->timeout() != std::chrono::milliseconds{25}) {
        std::cerr << "[unit] bootstrap config content mismatch" << std::endl;
        return false;
    }

    if (crynet::bootstrap::BootstrapConfig::from_text("timeout_ms nope\n").has_value()) {
        std::cerr << "[unit] bootstrap config should reject invalid timeout" << std::endl;
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

    if (!verify_bootstrap_config()) {
        return 1;
    }

    if (!verify_bootstrapper()) {
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
