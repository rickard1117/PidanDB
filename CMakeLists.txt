cmake_minimum_required(VERSION 3.10)

project(PidanDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)              # Compile as C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)     # Require C++17 support

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules")

find_package(TBB REQUIRED)
if (NOT TBB_FOUND)
    message(FATAL_ERROR "Cannot find Google Test Framework!")
endif()

include(SetupCxxFlags)

set(PIDAN_SRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PIDAN_SRC_INCLUDE_DIR})

option(BUILD_UNIT_TEST "build googletest unit tests" ON)
option(BUILD_BENCHMARK "build benchmark" ON)
option(USE_JEMALLOC "use jemalloc lib" OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (USE_JEMALLOC)
find_package(Jemalloc REQUIRED)
endif()

if (BUILD_UNIT_TEST)
    enable_testing()
    find_package(GTest REQUIRED)
    if (NOT GTest_FOUND)
        message(FATAL_ERROR "Cannot find Google Test Framework!")
    endif()
    add_subdirectory(test)
endif()

if (BUILD_BENCHMARK)
    find_package(GBenchmark REQUIRED)
    if (NOT GBENCHMARK_FOUND)
        message(FATAL_ERROR "Cannot find Google Benchmark Framework!")
    endif()
    add_subdirectory(benchmark)
endif()



add_subdirectory(src)