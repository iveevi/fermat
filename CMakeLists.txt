cmake_minimum_required(VERSION 3.20)

project(fermat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Color output always
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")

include_directories("vendor/termcolor/include")

file(GLOB_RECURSE SOURCES "source/*.cpp")

add_executable(fermat ${SOURCES})
target_link_libraries(fermat gccjit)
