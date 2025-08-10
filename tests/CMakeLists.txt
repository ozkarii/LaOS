cmake_minimum_required(VERSION 3.14)
project(tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
  
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
  memcpy_test
  memcpy_test.cc
  ../src/libc/memcpy.c
)

add_executable(
  snprintf_test
  snprintf_test.cc
  ../src/libc/snprintf.c
)

add_executable(
  strcmp_test
  strcmp_test.cc
  ../src/libc/strcmp.c
)

target_link_libraries(
  memcpy_test
  GTest::gtest_main
)

target_link_libraries(
  snprintf_test
  GTest::gtest_main
)

target_link_libraries(
  strcmp_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(memcpy_test)
gtest_discover_tests(snprintf_test)
gtest_discover_tests(strcmp_test)
