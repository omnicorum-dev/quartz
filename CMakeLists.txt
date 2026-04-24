cmake_minimum_required(VERSION 3.31)
project(Quartz)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(vendor/base-layer)

add_executable(QuartzTesting testing/main.cpp)
target_include_directories(QuartzTesting PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(QuartzTesting PUBLIC BaseLayer)

add_library(Quartz INTERFACE)
target_include_directories(Quartz INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(Quartz INTERFACE BaseLayer)