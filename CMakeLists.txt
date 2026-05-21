cmake_minimum_required(VERSION 3.28)

project(LaOS VERSION 0.0.1 LANGUAGES C ASM)

option(LAOS_TARGET_BOARD "The target board to build for (currently only 'qemu-raspi4b')" "qemu-raspi4b")

add_subdirectory(src)