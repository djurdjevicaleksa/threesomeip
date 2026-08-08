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

# Then the binary
add_executable( threesomeip
    src/test1.cpp
)

target_link_libraries( threesomeip
    PRIVATE
        threesomeip_impl
)


# Then the test binary

enable_testing()
add_subdirectory(tests)



# set(3rd_parties     "${CMAKE_SOURCE_DIR}/3rd_party")
# set(spd             "${3rd_parties}/spdlog")
# set(json            "${3rd_parties}/json")
# set(gtest           "${3rd_parties}/gtest")

# SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# add_subdirectory( ${spd} )
# add_subdirectory( ${gtest} )

# macro( build_application application main_file)

#     add_executable( ${application} )

#     target_sources( ${application}
#         PRIVATE
#             ${main_file}
#             src/udsocket.cpp
#     )

#     target_include_directories( ${application}
#         PRIVATE
#             include/
#             ${SPD}/include
#     )

#     target_compile_options( ${application}
#         PRIVATE
#             -Wall
#             -Wextra
#             -pedantic
#             -std=c++23
#             -O0
#     )

#     target_link_libraries( ${application}
#         PRIVATE
#             spdlog
#     )
# endmacro()

# add_library( threesomeip_impl )

# target_sources( threesomeip_impl 

# )

# # build_application( test1 src/test1.cpp)
# # build_application( test2 src/test2.cpp)
