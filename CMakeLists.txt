cmake_minimum_required(VERSION 3.20.0)

project(threesomeip)



SET(3RD_PARTIES "${CMAKE_SOURCE_DIR}/3rd_party")
add_subdirectory("${3RD_PARTIES}/json")



add_executable(threesomeip_runtime)




target_sources( threesomeip_runtime
    PRIVATE
        src/main_runtime.cpp
        src/runtime.cpp
        src/configurable.cpp
)

target_include_directories( threesomeip_runtime
    PRIVATE
        include/
)

target_compile_options( threesomeip_runtime
    PRIVATE
        -Wall
        -Wextra
        -pedantic
        -std=c++23
        -O0
)

target_link_libraries( threesomeip_runtime
    PRIVATE
        nlohmann_json
)