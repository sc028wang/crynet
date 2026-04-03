add_executable(crynet_unit_smoke
    unit/sources/smoke_suite.cpp
)

target_include_directories(crynet_unit_smoke PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(crynet_unit_smoke PRIVATE crynet_startup crynet_project_options)
add_test(NAME crynet_unit_smoke COMMAND crynet_unit_smoke)

add_executable(crynet_benchmark_smoke
    benchmark/sources/startup_smoke_benchmark.cpp
)

target_include_directories(crynet_benchmark_smoke PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(crynet_benchmark_smoke PRIVATE crynet_startup crynet_project_options)
