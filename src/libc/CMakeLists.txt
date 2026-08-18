set(TARGET libc)

if (NOT BUILD_TESTS)
  add_library(${TARGET}
    memcpy.c
    memset.c
    snprintf.c
    strcmp.c
    strcpy.c
    strlen.c
    vsnprintf.c
    wrapper.c
  )

  target_include_directories(${TARGET} 
    PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(${TARGET} PUBLIC LaOS::common)
else()
  add_library(${TARGET} INTERFACE)
endif()

add_library(LaOS::libc ALIAS ${TARGET})