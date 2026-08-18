add_library(common INTERFACE)

add_library(LaOS::common ALIAS common)

# Set 'bare metal' compile options if not building host tests
if (NOT BUILD_TESTS)
  target_compile_options(common INTERFACE
    -nostartfiles
    -ffreestanding
    -nostdlib
    -Wall
    -Wextra
    -march=armv8-a
    -mgeneral-regs-only
    -mno-outline-atomics
    -Werror=implicit-function-declaration
  )

  target_link_options(common INTERFACE
    -nostartfiles
  )

  target_include_directories(common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
endif()

set_target_properties(common PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED YES
  C_EXTENSIONS YES
)
