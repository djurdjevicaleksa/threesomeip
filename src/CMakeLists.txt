# IMPLEMENTATION

add_library( threesomeip_impl
    udsocket.cpp
    tcpsocket.cpp
    configurable.cpp
    active_object.cpp
    runtime_proxy.cpp
    runtime_stub.cpp
)

target_include_directories( threesomeip_impl
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries ( threesomeip_impl
    PUBLIC
        spdlog
        nlohmann_json
)

target_compile_options( threesomeip_impl
    PRIVATE
        -Wall -Wextra -Wpedantic -Wimplicit-fallthrough -Wshadow -Wconversion
        -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wundef -Wcast-align -Wcast-qual -Wold-style-cast
        -Woverloaded-virtual -Wnon-virtual-dtor -Wmisleading-indentation -Wduplicated-cond
        -Wduplicated-branches -Wlogical-op -Wuseless-cast
        -g3 -O0
)
