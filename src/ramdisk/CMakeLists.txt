add_library(ramdisk_loader STATIC ramdisk-load.c)

target_link_libraries(ramdisk_loader
  PRIVATE
    LaOS::common
    LaOS::kernel::utils
    LaOS::libc
)

target_include_directories(ramdisk_loader
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(LaOS::ramdisk::loader ALIAS ramdisk_loader)

# Custom command to compile the ramdisk generator C file with host compiler and generate the ramdisk binary
# using the same header as the kernel. This ensures that the ramdisk binary is generated with the correct format
# and can be loaded by the kernel.
# This command shall output the ramdisk-gen executable and ../user will use that to generate the ramdisk binary
# after userland programs are built.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ramdisk-gen
  COMMAND gcc
          -o ${CMAKE_CURRENT_BINARY_DIR}/ramdisk-gen
          ${CMAKE_CURRENT_SOURCE_DIR}/ramdisk-gen.c
          -I${CMAKE_CURRENT_SOURCE_DIR}/include
          $<LIST:TRANSFORM,$<TARGET_PROPERTY:LaOS::kernel::utils,INTERFACE_INCLUDE_DIRECTORIES>,PREPEND,-I>
  COMMAND_EXPAND_LISTS
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ramdisk-gen.c
  COMMENT "Compiling ramdisk generator"
)

add_custom_target(ramdisk_gen ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ramdisk-gen
)

set(LAOS_RAMDISK_GEN_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/ramdisk-gen PARENT_SCOPE)

