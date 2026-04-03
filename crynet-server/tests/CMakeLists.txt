cmake_minimum_required(VERSION 3.24)

project(crynet_tests VERSION 0.1.0 LANGUAGES CXX)

set(BUILD_TESTING ON CACHE BOOL "Build crynet tests" FORCE)

include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/crynet.cmake")
enable_testing()
include("${CRYNET_ROOT_DIR}/cmake/tests.cmake")

message(STATUS "Configured crynet tests ${PROJECT_VERSION} with C++${CMAKE_CXX_STANDARD}")