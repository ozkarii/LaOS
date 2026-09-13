add_subdirectory(fs)
add_subdirectory(utils)

if (NOT BUILD_TESTS)
  add_subdirectory(drivers)
  add_subdirectory(memory)
  add_subdirectory(platform)
  add_subdirectory(system)

  set(TARGET kernel)

  add_executable(${TARGET}
    console.c
    main.c
    isr.c
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
      LaOS::kernel::utils
      LaOS::kernel::drivers
      LaOS::kernel::system
      LaOS::kernel::memory
      LaOS::kernel::platform
      LaOS::kernel::fs
      LaOS::ramdisk::loader
  )
endif()