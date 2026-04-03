add_library(crynet_startup STATIC
    "${CRYNET_ROOT_DIR}/startup/app/sources/startup_app.cpp"
)

add_library(crynet::startup ALIAS crynet_startup)

target_include_directories(crynet_startup PUBLIC "${CRYNET_ROOT_DIR}")
target_link_libraries(crynet_startup PUBLIC crynet::core crynet_project_options Threads::Threads)

add_executable(crynet_server
    "${CRYNET_ROOT_DIR}/startup/app/sources/main.cpp"
)

target_link_libraries(crynet_server PRIVATE crynet::startup crynet_project_options)