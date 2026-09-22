cmake_minimum_required(VERSION 3.20.0)
project(threesomeip)

set(3rd_parties     "${CMAKE_SOURCE_DIR}/3rd_party")
set(spd             "${3rd_parties}/spdlog")
set(json            "${3rd_parties}/json")
set(gtest           "${3rd_parties}/gtest")
set(ftxui           "${3rd_parties}/ftxui")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(DEPLOYMENT_PATH "${CMAKE_SOURCE_DIR}/deployment/build_output")

file(REMOVE_RECURSE ${DEPLOYMENT_PATH})


macro(deploy target_name)
    add_custom_target(deploy_${target_name} ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOYMENT_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:${target_name}> ${DEPLOYMENT_PATH}/
        COMMAND ${CMAKE_COMMAND} -E echo "Deployed ${target_name} to ${DEPLOYMENT_PATH}"
        DEPENDS ${target_name}
        COMMAND echo "Deploying ${target_name}"
    )
endmacro()


macro(build_application application_name)
    add_executable( ${application_name}
        "src/${application_name}.cpp"
    )

    target_link_libraries( ${application_name}
        PRIVATE
            threesomeip_impl
            spdlog
    )

    target_compile_options( ${application_name}
        PRIVATE
            -Wall -Wextra -Wpedantic -Wimplicit-fallthrough -Wshadow -Wconversion
            -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wundef -Wcast-align -Wcast-qual -Wold-style-cast
            -Woverloaded-virtual -Wnon-virtual-dtor -Wmisleading-indentation -Wduplicated-cond
            -Wduplicated-branches -Wlogical-op -Wuseless-cast
            -g3 -O0
    )

    deploy(${application_name})
endmacro()


# Build dependencies
add_subdirectory(${spd})
add_subdirectory(${gtest})
add_subdirectory(${ftxui})
add_subdirectory(${json})


# Global project configuration
add_compile_definitions(
    SOCKET_DEBUG
    RUNTIME_COMM_DEBUG
)

# Compile the implementation first
add_subdirectory(src)
add_subdirectory(example)

# Then the test binary
enable_testing()
add_subdirectory(tests)


# Then individual applications
build_application(main_runtime)

build_application(main_calculator_proxy)
target_link_libraries( main_calculator_proxy
    PRIVATE
        calculator_proxy_lib
        ftxui
)

build_application(main_calculator_stub)
target_link_libraries( main_calculator_stub
    PRIVATE
        calculator_stub_lib
)
