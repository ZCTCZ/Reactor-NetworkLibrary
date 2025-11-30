cmake_minimum_required(VERSION 4.0.0)
project(ReactorEchoServer VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(src)
add_subdirectory(example)
add_subdirectory(StressTest)