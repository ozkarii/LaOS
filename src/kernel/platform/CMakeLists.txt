add_library(kernel_platform STATIC
  spinlock.c
)

add_library(LaOS::kernel::platform ALIAS kernel_platform)

target_include_directories(kernel_platform
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kernel_platform
  PUBLIC
    LaOS::common
)