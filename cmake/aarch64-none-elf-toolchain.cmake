# CMake toolchain file for aarch64-none-elf (assumes aarch64-none-elf-* are in PATH)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Use cross compilers from PATH
set(CMAKE_C_COMPILER aarch64-none-elf-gcc)
set(CMAKE_CXX_COMPILER aarch64-none-elf-g++)
set(CMAKE_ASM_COMPILER aarch64-none-elf-gcc)
set(CMAKE_AR aarch64-none-elf-ar)
set(CMAKE_OBJCOPY aarch64-none-elf-objcopy)
set(CMAKE_OBJDUMP aarch64-none-elf-objdump)
set(CMAKE_NM aarch64-none-elf-nm)
set(CMAKE_STRIP aarch64-none-elf-strip)

# Avoid trying to run compiled executables on the build machine
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search behavior: programs on host, libraries/includes/libs on toolchain
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Print a summary
message(STATUS "Using cross compiler: ${CMAKE_C_COMPILER}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")
