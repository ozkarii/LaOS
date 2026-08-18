add_library(kernel_drivers STATIC
  gic.c
  pl011.c
)

add_library(LaOS::kernel::drivers ALIAS kernel_drivers)

target_include_directories(kernel_drivers
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kernel_drivers
  PRIVATE
    LaOS::kernel::platform
    LaOS::common
    LaOS::libc
)