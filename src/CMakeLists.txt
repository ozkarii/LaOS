add_subdirectory(common)
add_subdirectory(kernel)
add_subdirectory(libc)

if (NOT BUILD_TESTS)
  add_subdirectory(ramdisk)
  add_subdirectory(user)
endif()