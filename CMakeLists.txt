cmake_minimum_required(VERSION 3.14)
project(RateLimiterProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(googletest)

add_executable(run_tests rate_limiter_tests.cpp)

target_link_libraries(run_tests gtest gtest_main)

enable_testing()
include(GoogleTest)

# Discover tests with a longer timeout to prevent errors
gtest_discover_tests(run_tests DISCOVERY_TIMEOUT 10)