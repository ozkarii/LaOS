function(add_libc_test test_name test_source libc_source)
  add_executable(${test_name} ${test_source} ${CMAKE_CURRENT_SOURCE_DIR}/../../src/libc/${libc_source})
  
  target_link_libraries(${test_name}
    GTest::gtest_main
  )
  
  target_include_directories(${test_name}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/../../src/libc
  )
  
  gtest_discover_tests(${test_name})
endfunction()

add_libc_test(memcpy_test memcpy_test.cc memcpy.c)
add_libc_test(snprintf_test snprintf_test.cc snprintf.c)
add_libc_test(strcmp_test strcmp_test.cc strcmp.c)
add_libc_test(strlen_test strlen_test.cc strlen.c)