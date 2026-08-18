add_library(kernel_system STATIC
  io.c
  process.c
  sched.c
  sem.c
  serial-buffer.c
  syscall-kernel.c
)

add_library(LaOS::kernel::system ALIAS kernel_system)

target_include_directories(kernel_system
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kernel_system
  PUBLIC  
    LaOS::common
    LaOS::libc
    LaOS::kernel::platform
  PRIVATE
    LaOS::kernel::drivers
    LaOS::kernel::utils
    LaOS::kernel::memory
    LaOS::kernel::fs
)