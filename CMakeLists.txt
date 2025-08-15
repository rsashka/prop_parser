cmake_minimum_required(VERSION 3.10)
project(prop_parser)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create library
add_library(prop_parser STATIC
    src/PropertyParser.cpp
)

# Include directories
target_include_directories(prop_parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# GoogleTest setup
enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Create test executable
add_executable(PropertyParserTests tests/PropertyParserTests.cpp)
target_link_libraries(PropertyParserTests prop_parser GTest::gtest_main)

# Add test
add_test(NAME PropertyParserTests COMMAND PropertyParserTests)