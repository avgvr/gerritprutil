# Copyright (c) 2026, Alexey Gavrilov


add_executable(lexer-tests
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer_testing.cpp
    ${CMAKE_SOURCE_DIR}/src/lexer.cpp
)
target_link_libraries(lexer-tests
    PUBLIC GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(lexer-tests)
