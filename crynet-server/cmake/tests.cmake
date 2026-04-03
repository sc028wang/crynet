add_executable(crynet_unit_smoke
    "${CRYNET_ROOT_DIR}/tests/unit/sources/smoke_suite.cpp"
)

target_include_directories(crynet_unit_smoke PRIVATE "${CRYNET_ROOT_DIR}")
target_link_libraries(crynet_unit_smoke PRIVATE crynet::startup crynet_project_options)
add_test(NAME crynet_unit_smoke COMMAND crynet_unit_smoke)

add_executable(crynet_benchmark_smoke
    "${CRYNET_ROOT_DIR}/tests/benchmark/sources/startup_smoke_benchmark.cpp"
)

target_include_directories(crynet_benchmark_smoke PRIVATE "${CRYNET_ROOT_DIR}")
target_link_libraries(crynet_benchmark_smoke PRIVATE crynet::startup crynet_project_options)

add_executable(crynet_benchmark_actor_echo
    "${CRYNET_ROOT_DIR}/tests/benchmark/sources/actor_echo_benchmark.cpp"
)

target_include_directories(crynet_benchmark_actor_echo PRIVATE "${CRYNET_ROOT_DIR}")
target_link_libraries(crynet_benchmark_actor_echo PRIVATE crynet::core crynet_project_options)