cmake_minimum_required(VERSION 3.20)
project(CryNet
    VERSION     0.1.0
    DESCRIPTION "High-performance C++20 actor-model server framework"
    LANGUAGES   CXX
)

# ---------------------------------------------------------------------------
# C++20 standard – required for coroutines, concepts, and std::any
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ---------------------------------------------------------------------------
# Compiler warnings
# ---------------------------------------------------------------------------
add_compile_options(
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# ---------------------------------------------------------------------------
# Threads (pthreads on Linux/macOS, native threads on Windows)
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# crynet – core static library
# ---------------------------------------------------------------------------
add_library(crynet STATIC
    src/Mailbox.cpp
    src/Actor.cpp
    src/Scheduler.cpp
)

target_include_directories(crynet
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(crynet PUBLIC Threads::Threads)

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------
add_executable(ping_pong examples/ping_pong.cpp)
target_link_libraries(ping_pong PRIVATE crynet)

# ---------------------------------------------------------------------------
# Install rules (optional – useful when embedding CryNet in another project)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS crynet
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/crynet
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
