cmake_minimum_required(VERSION 3.14)
project(RateLimiterProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()
add_subdirectory(googletest)

# --- Define the main demo executable ---
# This new line builds your main.cpp into an executable named "main"
add_executable(main main.cpp)

# --- Define the test executable ---
add_executable(run_tests rate_limiter_tests.cpp)
target_link_libraries(run_tests gtest gtest_main)

# Discover tests for the test runner
include(GoogleTest)
gtest_discover_tests(run_tests DISCOVERY_TIMEOUT 10)
