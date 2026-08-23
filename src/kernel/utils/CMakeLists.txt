# Standalone library for kernel utilities, only depends on libc

add_library(kernel_utils STATIC
  path.c
  elf-loader.c
)
add_library(LaOS::kernel::utils ALIAS kernel_utils)

target_include_directories(kernel_utils
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kernel_utils
  PRIVATE
    LaOS::common
    LaOS::libc
)