cmake_minimum_required(VERSION 3.16)
project(Zippy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME} src/main.cpp src/zippy_core.cpp src/zippy_routing.cpp src/zippy_utils.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include/)