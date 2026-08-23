
add_executable(path_test
  path_test.cc
)

target_link_libraries(path_test
  PRIVATE
    GTest::gtest_main
    LaOS::kernel::utils
)

gtest_discover_tests(path_test)