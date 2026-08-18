add_executable(vfs_test 
  vfs_test.cc
)

target_link_libraries(vfs_test
  PRIVATE
    GTest::gtest_main
    LaOS::kernel::fs
)

gtest_discover_tests(vfs_test)