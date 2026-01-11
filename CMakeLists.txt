cmake_minimum_required(VERSION 3.14)
project(prop_parser)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create library
add_library(prop_parser STATIC
    PropertyParser.cpp
)

# Include directories
target_include_directories(prop_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# GoogleTest setup
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Create test executable
add_executable(PropertyParserTests PropertyParserTests.cpp)
target_link_libraries(PropertyParserTests prop_parser GTest::gtest_main)

# Вместо ctest: делаем цель `test`, которая напрямую запускает бинарник юнит-тестов.
add_custom_target(run_test
    COMMAND PropertyParserTests
    DEPENDS PropertyParserTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
