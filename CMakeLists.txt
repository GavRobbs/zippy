cmake_minimum_required(VERSION 3.16)
project(Zippy)

option(ZIPPY_BUILD_TESTS "Builds the tests" ON)
option(ZIPPY_BUILD_DEMO " Builds the demo" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3 -march=native")

include_directories(include/)

add_library(${PROJECT_NAME} src/zippy_core.cpp src/zippy_routing.cpp src/zippy_utils.cpp)

if(ZIPPY_BUILD_DEMO)
    add_executable(ZippyDemo src/main.cpp)
    target_link_libraries(ZippyDemo ${PROJECT_NAME})
else()
    message("The demo will not be compiled, as ZIPPY_BUILD_DEMO is currently OFF. Turn it ON if you want the demo.")
endif()

if(ZIPPY_BUILD_TESTS)
    add_subdirectory(tests)
else()
    message("The tests will not be compiled, as ZIPPY_BUILD_TESTS is currently OFF. Turn it ON if you want the tests to compile.")
endif()