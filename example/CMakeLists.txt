add_library( calculator_proxy_lib
    src/calculator_proxy.cpp
)

target_include_directories( calculator_proxy_lib
    PUBLIC
        include/
)

target_compile_options( calculator_proxy_lib
    PRIVATE
        -Wall -Wextra -Wpedantic -Wimplicit-fallthrough -Wshadow -Wconversion
        -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wundef -Wcast-align -Wcast-qual -Wold-style-cast
        -Woverloaded-virtual -Wnon-virtual-dtor -Wmisleading-indentation -Wduplicated-cond
        -Wduplicated-branches -Wlogical-op -Wuseless-cast
        -g3 -O0
)

target_link_libraries( calculator_proxy_lib
    PUBLIC
        threesomeip_impl
)



add_library( calculator_stub_lib
    src/calculator_stub_impl.cpp
)

target_include_directories( calculator_stub_lib
    PUBLIC
        include/
)

target_compile_options( calculator_stub_lib
    PRIVATE
        -Wall -Wextra -Wpedantic -Wimplicit-fallthrough -Wshadow -Wconversion
        -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wundef -Wcast-align -Wcast-qual -Wold-style-cast
        -Woverloaded-virtual -Wnon-virtual-dtor -Wmisleading-indentation -Wduplicated-cond
        -Wduplicated-branches -Wlogical-op -Wuseless-cast
        -g3 -O0
)

target_link_libraries( calculator_stub_lib
    PUBLIC
        threesomeip_impl
)