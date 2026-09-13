cmake_minimum_required(VERSION 3.28)

project(LaOS VERSION 0.0.1 LANGUAGES C CXX ASM)

option(BUILD_TESTS "Build host tests" OFF)

include(cmake/Kconfig.cmake)

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()

add_subdirectory(src)
