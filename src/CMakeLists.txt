add_library( threesomeip_impl
    udsocket.cpp
)

target_include_directories( threesomeip_impl
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries (threesomeip_impl
    PUBLIC
        spdlog
)