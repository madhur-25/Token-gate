# Required CMake version and project name
cmake_minimum_required(VERSION 3.14)
project(RateLimiterTests)

# Use the C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Download and configure Google Test ---
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- Define the test executable ---
# This creates an executable named "run_tests" from your test file.
add_executable(run_tests rate_limiter_tests.cpp)

# Link the test executable with the Google Test library
target_link_libraries(run_tests GTest::gtest_main)

# --- Enable testing ---
enable_testing()
include(GoogleTest)
gtest_discover_tests(run_tests)