add_library(crynet_core STATIC
    "${CRYNET_ROOT_DIR}/core/base/sources/build_config.cpp"
)

add_library(crynet::core ALIAS crynet_core)

target_include_directories(crynet_core PUBLIC "${CRYNET_ROOT_DIR}")
target_link_libraries(crynet_core PUBLIC crynet_project_options)