cmake_minimum_required(VERSION 3.28)

project(LaOS VERSION 0.0.1 LANGUAGES C CXX ASM)

option(BUILD_TESTS "Build host tests only" OFF)

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Forcing debug build")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if (BUILD_TESTS)
    add_subdirectory(tests)
else()
    add_subdirectory(src)
endif()
