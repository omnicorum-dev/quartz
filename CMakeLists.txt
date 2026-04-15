cmake_minimum_required(VERSION 3.31)
project(Quartz)

set(CMAKE_CXX_STANDARD 17)

add_executable(Quartz testing/main.cpp)
target_include_directories(Quartz PUBLIC base-layer ./)
