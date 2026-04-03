add_library(crynet_base STATIC
    base/sources/build_config.cpp
)

target_include_directories(crynet_base PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(crynet_base PUBLIC crynet_project_options)
