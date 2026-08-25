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

# Build dependencies
add_subdirectory(${spd})
add_subdirectory(${gtest})
add_subdirectory(${ftxui})


# Compile the implementation first
add_subdirectory(src)

add_subdirectory(example)

add_executable( calculator_proxy
    "src/main_calculator_proxy.cpp"
)

target_link_libraries( calculator_proxy
    PRIVATE
        calculator_proxy_lib
        ftxui
)

add_executable( calculator_stub
    "src/main_calculator_stub.cpp"
)

target_link_libraries( calculator_stub
    PRIVATE
        calculator_stub_lib
)

# Then the test binary
enable_testing()
add_subdirectory(tests)