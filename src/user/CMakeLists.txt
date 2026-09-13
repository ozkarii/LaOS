add_subdirectory(common)
add_subdirectory(init)
add_subdirectory(sh)

add_custom_target(ramdisk ALL
  DEPENDS ${LAOS_RAMDISK_GEN_EXECUTABLE} sh init
  COMMAND ${LAOS_RAMDISK_GEN_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/ramdisk.img
          $<TARGET_FILE:sh> /bin/sh
          $<TARGET_FILE:init> /sbin/init
  COMMAND_EXPAND_LISTS
  COMMENT "Generating ramdisk"
)