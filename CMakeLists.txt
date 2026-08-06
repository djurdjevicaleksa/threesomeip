cmake_minimum_required(VERSION 3.20.0)
project(threesomeip)


SET(3RD_PARTIES "${CMAKE_SOURCE_DIR}/3rd_party")
SET(SPD     "${3RD_PARTIES}/spdlog")
SET(JSON    "${3RD_PARTIES}/json")

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

add_subdirectory( ${SPD} )

macro( build_application application main_file)

    add_executable( ${application} )

    target_sources( ${application}
        PRIVATE
            ${main_file}
            src/udsocket.cpp
    )

    target_include_directories( ${application}
        PRIVATE
            include/
            ${SPD}/include
    )

    target_compile_options( ${application}
        PRIVATE
            -Wall
            -Wextra
            -pedantic
            -std=c++23
            -O0
    )

    target_link_libraries( ${application}
        PRIVATE
            spdlog
    )
endmacro()

build_application( test1 src/test1.cpp)
build_application( test2 src/test2.cpp)
