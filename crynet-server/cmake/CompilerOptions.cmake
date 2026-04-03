include_guard(GLOBAL)

function(crynet_configure_global_options)
    if(TARGET crynet_project_options)
        return()
    endif()

    option(CRYNET_ENABLE_ASAN "Enable AddressSanitizer" OFF)
    option(CRYNET_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
    option(CRYNET_ENABLE_WERROR "Treat warnings as errors" OFF)

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
endfunction()
