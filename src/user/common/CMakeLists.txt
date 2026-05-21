set(TARGET user_common)

add_library(${TARGET}
  STATIC
    crt.s
    syscall-user.c
)

target_link_libraries(${TARGET} PUBLIC LaOS::common)

target_link_options(${TARGET} 
  PUBLIC
    "LINKER:-T,${CMAKE_CURRENT_SOURCE_DIR}/link.ld"
)

add_library(LaOS::common_user ALIAS user_common)