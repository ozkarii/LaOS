add_executable(
  memcpy_test
  memcpy_test.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/libc/memcpy.c
)

add_executable(
  snprintf_test
  snprintf_test.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/libc/snprintf.c
)

add_executable(
  strcmp_test
  strcmp_test.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/libc/strcmp.c
)

add_executable(
  strlen_test
  strlen_test.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/libc/strlen.c
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

target_link_libraries(
  strlen_test
  GTest::gtest_main
)


target_include_directories(memcpy_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR/../../src/libc/include}
)

target_include_directories(snprintf_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR/../../src/libc/include}
)

target_include_directories(strcmp_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR/../../src/libc/include}
)

target_include_directories(strlen_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR/../../src/libc/include}
)

gtest_discover_tests(memcpy_test)
gtest_discover_tests(snprintf_test)
gtest_discover_tests(strcmp_test)
gtest_discover_tests(strlen_test)