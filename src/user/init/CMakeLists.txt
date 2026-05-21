set(TARGET init)

add_executable(${TARGET}
  main.c
)

target_link_libraries(${TARGET} 
  PRIVATE
    LaOS::common_user 
)