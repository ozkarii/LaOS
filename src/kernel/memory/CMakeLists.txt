add_library(kernel_memory STATIC
  memory.c
  mmu.c
)

add_library(LaOS::kernel::memory ALIAS kernel_memory)

target_include_directories(kernel_memory
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kernel_memory
  PRIVATE
    LaOS::common
    LaOS::libc
    LaOS::kernel::utils
    LaOS::kernel::platform
)