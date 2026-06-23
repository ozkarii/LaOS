cmake_minimum_required(VERSION 3.28)

project(LaOS VERSION 0.0.1 LANGUAGES C CXX ASM)

option(BUILD_TESTS "Build host tests only" OFF)

if (BUILD_TESTS)
    add_subdirectory(tests)
else()
    add_subdirectory(src)
endif()
