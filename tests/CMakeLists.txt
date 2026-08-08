add_executable( threesomeip_test
    test_serdes.cpp
)

target_link_libraries( threesomeip_test
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        threesomeip_impl
)

include(GoogleTest)
gtest_discover_tests(threesomeip_test)

add_custom_command(
    TARGET threesomeip_test
    POST_BUILD
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
)