cmake_minimum_required(VERSION 3.31)
project(Quartz)

set(CMAKE_CXX_STANDARD 17)

add_executable(app testing/main.cpp)
target_include_directories(app PUBLIC vendor/base-layer vendor ./)

add_library(Quartz)
target_sources(Quartz PRIVATE quartz.cpp)
target_include_directories(Quartz PUBLIC vendor/base-layer vendor ./)