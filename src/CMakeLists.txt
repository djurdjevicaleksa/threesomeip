# IMPLEMENTATION

add_library( threesomeip_impl
    udsocket.cpp
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
)

target_compile_options( threesomeip_impl
    PRIVATE
        -Wall
        -Wextra
        -pedantic
        -O0
        -g3
        -Wimplicit-fallthrough
)

# APPLICATIONS

macro(build_application application_name)
    add_executable( ${application_name} 
        "${application_name}.cpp"
    )

    target_link_libraries( ${application_name}
        PRIVATE
            threesomeip_impl
    )

    target_compile_options( ${application_name}
        PRIVATE
            -Wall
            -Wextra
            -pedantic
            -O0
            -g3
            -Wimplicit-fallthrough
    )
endmacro()

# build_application(runtime)
# build_application(client)