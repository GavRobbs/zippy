cmake_minimum_required(VERSION 3.16)
project(Zippy)

option(ZIPPY_BUILD_TESTS "Builds the tests" ON)
option(ZIPPY_BUILD_DEMO " Builds the demo" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3 -march=native")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)

include_directories(include/)

add_library(${PROJECT_NAME} SHARED src/zippy_core.cpp src/zippy_routing.cpp src/zippy_utils.cpp src/socket_buffer_reader.cpp src/socket_buffer_writer.cpp src/zippy_exceptions.cpp src/extension_factory.cpp)
target_link_libraries(${PROJECT_NAME} dl Threads::Threads ${LIBUV_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUV_INCLUDE_DIRS})

add_library(ZippyConsoleLogger SHARED src/extensions/console_logger.cpp)
target_link_libraries(ZippyConsoleLogger ${PROJECT_NAME} Threads::Threads)
set_target_properties(ZippyConsoleLogger PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/extensions/")

add_library(ZippyFileLogger SHARED src/extensions/file_logger.cpp)
target_link_libraries(ZippyFileLogger ${PROJECT_NAME} Threads::Threads)
set_target_properties(ZippyFileLogger PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/extensions/")

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