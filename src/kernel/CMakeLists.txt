set(TARGET kernel)

add_executable(${TARGET}
  gic.c
  serial-buffer.c
  io.c
  pl011.c
  sched.c
  sem.c
  sp804.c
  spinlock.c
  sys-timer.c
  process.c
  console.c
  main.c
  mmu.c
  memory.c
  vfs.c
  isr.c
  elf-loader.c
  ramfs.c
  syscall-kernel.c
  el1_vectors.s
  el2_vectors.s
  el3_vectors.s
  start.s
)

target_link_options(${TARGET} 
  PRIVATE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/link.ld
)

target_include_directories(${TARGET} 
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET} 
  PRIVATE
    LaOS::common
    LaOS::libc
)
