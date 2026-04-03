add_library(crynet_core STATIC
    "${CRYNET_ROOT_DIR}/core/base/sources/build_config.cpp"
    "${CRYNET_ROOT_DIR}/core/base/sources/logger.cpp"
    "${CRYNET_ROOT_DIR}/core/actor/sources/actor_system.cpp"
    "${CRYNET_ROOT_DIR}/core/bootstrap/sources/bootstrap_config.cpp"
    "${CRYNET_ROOT_DIR}/core/bootstrap/sources/bootstrapper.cpp"
    "${CRYNET_ROOT_DIR}/core/monitor/sources/monitor_runner.cpp"
    "${CRYNET_ROOT_DIR}/core/metrics/sources/metrics_system.cpp"
    "${CRYNET_ROOT_DIR}/core/actor/sources/handle_registry.cpp"
    "${CRYNET_ROOT_DIR}/core/actor/sources/message.cpp"
    "${CRYNET_ROOT_DIR}/core/actor/sources/message_queue.cpp"
    "${CRYNET_ROOT_DIR}/core/actor/sources/session_manager.cpp"
    "${CRYNET_ROOT_DIR}/core/actor/sources/service_context.cpp"
    "${CRYNET_ROOT_DIR}/core/scheduler/sources/worker_monitor.cpp"
    "${CRYNET_ROOT_DIR}/core/scheduler/sources/ready_queue.cpp"
    "${CRYNET_ROOT_DIR}/core/scheduler/sources/worker_loop.cpp"
    "${CRYNET_ROOT_DIR}/core/scheduler/sources/worker_runner.cpp"
    "${CRYNET_ROOT_DIR}/core/scheduler/sources/worker_scheduler.cpp"
    "${CRYNET_ROOT_DIR}/core/scheduler/timer/sources/timer_queue.cpp"
)

add_library(crynet::core ALIAS crynet_core)

target_include_directories(crynet_core PUBLIC "${CRYNET_ROOT_DIR}")
target_link_libraries(crynet_core PUBLIC crynet_project_options)