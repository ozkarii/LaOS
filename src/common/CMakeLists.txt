add_library(common INTERFACE)

target_compile_options(common INTERFACE
  -nostartfiles
  -ffreestanding
  -nostdlib
  -Wall
  -Wextra
  -march=armv8-a
  -mgeneral-regs-only
  -mno-outline-atomics
)

set_target_properties(common PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED YES
  C_EXTENSIONS YES
)

target_link_options(common INTERFACE
  -nostartfiles
)

add_library(LaOS::common ALIAS common)