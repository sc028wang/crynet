cmake_minimum_required(VERSION 3.24)

project(crynet VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(CTest)
find_package(Threads REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerOptions)

crynet_configure_global_options()

set(CRYNET_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "crynet root directory")
set(CRYNET_V8_INCLUDE_DIR "${CRYNET_ROOT_DIR}/third-party/v8/includes" CACHE INTERNAL "Bundled V8 include directory")
set(CRYNET_V8_MONOLITH_LIB "${CRYNET_ROOT_DIR}/third-party/v8/mac/libv8_monolith.a" CACHE INTERNAL "Bundled V8 monolith archive")

if(EXISTS "${CRYNET_V8_INCLUDE_DIR}" AND EXISTS "${CRYNET_V8_MONOLITH_LIB}")
    add_library(crynet_v8_vendor INTERFACE)
    target_include_directories(crynet_v8_vendor SYSTEM INTERFACE "${CRYNET_V8_INCLUDE_DIR}")
    target_link_libraries(crynet_v8_vendor INTERFACE "${CRYNET_V8_MONOLITH_LIB}")
    message(STATUS "Bundled V8 detected at ${CRYNET_V8_MONOLITH_LIB}")
else()
    message(STATUS "Bundled V8 not found; submodules will configure without the V8 runtime archive")
endif()

add_subdirectory(core)
add_subdirectory(engine)
add_subdirectory(services)
add_subdirectory(startup)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

message(STATUS "Configured crynet ${PROJECT_VERSION} with C++${CMAKE_CXX_STANDARD}")
