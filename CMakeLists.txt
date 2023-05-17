cmake_minimum_required(VERSION 3.20)

project(fermat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Color output always
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")

set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(vendor/benchmark REQUIRED)

include_directories("vendor/termcolor/include")
include_directories("source")

file(GLOB_RECURSE SOURCES "source/*.cpp")

add_library(fermatlib STATIC ${SOURCES})

add_executable(fermat fermat.cpp)
target_link_libraries(fermat fermatlib gccjit)

add_executable(bench testing/bench.cpp)
target_link_libraries(bench fermatlib gccjit benchmark::benchmark)
