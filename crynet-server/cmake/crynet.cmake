set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED CRYNET_ROOT_DIR)
    get_filename_component(CRYNET_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

option(CRYNET_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(CRYNET_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(CRYNET_ENABLE_WERROR "Treat warnings as errors" OFF)

include(CTest)
find_package(Threads REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${CRYNET_ROOT_DIR}/cmake")

if(NOT TARGET crynet_project_options)
    add_library(crynet_project_options INTERFACE)
    target_compile_features(crynet_project_options INTERFACE cxx_std_20)

    target_compile_options(
        crynet_project_options
        INTERFACE
            $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall;-Wextra;-Wpedantic;-Wshadow>
            $<$<BOOL:${CRYNET_ENABLE_WERROR}>:-Werror>
    )

    if(CRYNET_ENABLE_ASAN)
        target_compile_options(crynet_project_options INTERFACE -fsanitize=address)
        target_link_options(crynet_project_options INTERFACE -fsanitize=address)
    endif()

    if(CRYNET_ENABLE_UBSAN)
        target_compile_options(crynet_project_options INTERFACE -fsanitize=undefined)
        target_link_options(crynet_project_options INTERFACE -fsanitize=undefined)
    endif()

    target_compile_definitions(
        crynet_project_options
        INTERFACE
            CRYNET_PROJECT_NAME="crynet"
            $<$<PLATFORM_ID:Darwin>:CRYNET_PLATFORM_MACOS=1>
            $<$<PLATFORM_ID:Linux>:CRYNET_PLATFORM_LINUX=1>
    )
endif()

set(CRYNET_ROOT_DIR "${CRYNET_ROOT_DIR}" CACHE INTERNAL "crynet root directory")
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

include("${CRYNET_ROOT_DIR}/cmake/core.cmake")
include("${CRYNET_ROOT_DIR}/cmake/engine.cmake")
include("${CRYNET_ROOT_DIR}/cmake/services.cmake")
include("${CRYNET_ROOT_DIR}/cmake/startup.cmake")