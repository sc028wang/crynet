if(TARGET crynet_v8_vendor)
    add_library(crynet_v8_engine STATIC
        "${CRYNET_ROOT_DIR}/engine/v8/engine/sources/v8_engine.cpp"
    )
    add_library(crynet::engine ALIAS crynet_v8_engine)
    target_include_directories(crynet_v8_engine PUBLIC "${CRYNET_ROOT_DIR}")
    target_link_libraries(crynet_v8_engine PUBLIC crynet_project_options crynet_v8_vendor)
else()
    add_library(crynet_v8_engine INTERFACE)
    add_library(crynet::engine ALIAS crynet_v8_engine)
    target_link_libraries(crynet_v8_engine INTERFACE crynet_project_options)
endif()