add_library(crynet_startup STATIC
    app/sources/startup_app.cpp
)

target_include_directories(crynet_startup PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(crynet_startup PUBLIC crynet_base crynet_project_options Threads::Threads)

add_executable(crynet_server
    app/sources/main.cpp
)

target_link_libraries(crynet_server PRIVATE crynet_startup crynet_project_options)
