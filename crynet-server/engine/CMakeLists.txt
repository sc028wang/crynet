if(TARGET crynet_v8_vendor)
    add_library(crynet_v8_engine STATIC
        v8/engine/sources/v8_engine.cpp
    )
    target_include_directories(crynet_v8_engine PUBLIC "${CMAKE_SOURCE_DIR}")
    target_link_libraries(crynet_v8_engine PUBLIC crynet_project_options crynet_v8_vendor)
else()
    add_library(crynet_v8_engine INTERFACE)
    target_link_libraries(crynet_v8_engine INTERFACE crynet_project_options)
endif()
