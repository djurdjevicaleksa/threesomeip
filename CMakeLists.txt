cmake_minimum_required(VERSION 3.20.0)
project(threesomeip)

set(3rd_parties     "${CMAKE_SOURCE_DIR}/3rd_party")
set(spd             "${3rd_parties}/spdlog")
set(json            "${3rd_parties}/json")
set(gtest           "${3rd_parties}/gtest")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Build dependencies
add_subdirectory(${spd})
add_subdirectory(${gtest})


# Compile the implementation first
add_subdirectory(src)


# Then the test binary
enable_testing()
add_subdirectory(tests)