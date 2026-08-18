add_library(kernel_fs STATIC
  vfs.c
  ramfs.c
)

add_library(LaOS::kernel::fs ALIAS kernel_fs)

target_include_directories(kernel_fs
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kernel_fs
  PRIVATE
    LaOS::libc
  PUBLIC
    LaOS::common
    LaOS::kernel::utils
)