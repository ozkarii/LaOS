# Generates one autoconf.h per subsystem from the Kconfig tree, so each
# subsystem's CMakeLists.txt only exposes its own CONFIG_* header to itself
# (and whatever links its public includes).
#
# Usage:
#   include(cmake/Kconfig.cmake)   # once, from the top-level CMakeLists.txt
#   laos_use_kconfig(<target>)     # once per subsystem that has a Kconfig file

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(LAOS_KCONFIG_ROOT ${CMAKE_SOURCE_DIR}/Kconfig)
set(LAOS_DOTCONFIG ${CMAKE_SOURCE_DIR}/config/.config)
set(LAOS_DEFCONFIG ${CMAKE_SOURCE_DIR}/config/defconfig)
set(LAOS_GENCONFIG_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/kconfig/genconfig.py)
set(LAOS_CONFIG_OUT_DIR ${CMAKE_BINARY_DIR}/config)

# .config is user-local and gitignored, but menuconfig/genconfig need
# config/ to already exist to write/read it there.
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/config)

# Give config/.config an actual build rule, seeding it from config/defconfig
# the first time it's needed. Without this, a fresh checkout has no .config
# and no rule to make it, and Make refuses to build anything that depends
# on it ("No rule to make target"). Deliberately has NO dependency on
# defconfig: once .config exists, this rule is satisfied forever, so a later
# change to defconfig never silently overwrites a developer's local .config
# (they get that by re-running the `defconfig` target explicitly, see below).
add_custom_command(
    OUTPUT ${LAOS_DOTCONFIG}
    COMMAND ${CMAKE_COMMAND} -E copy ${LAOS_DEFCONFIG} ${LAOS_DOTCONFIG}
    COMMENT "Seeding config/.config from config/defconfig"
)

file(GLOB_RECURSE LAOS_SUBSYSTEM_KCONFIGS CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*/Kconfig
    ${CMAKE_SOURCE_DIR}/src/*/*/Kconfig
    ${CMAKE_SOURCE_DIR}/src/*/*/*/Kconfig
)

set(LAOS_KCONFIG_OUTPUTS "")
foreach(_kc ${LAOS_SUBSYSTEM_KCONFIGS})
    get_filename_component(_dir ${_kc} DIRECTORY)
    file(RELATIVE_PATH _rel ${CMAKE_SOURCE_DIR} ${_dir})
    # Keep in sync with header_name_for() in scripts/kconfig/genconfig.py
    string(REGEX REPLACE "^src/" "" _rel_no_src ${_rel})
    string(REPLACE "/" "_" _header_name ${_rel_no_src})
    list(APPEND LAOS_KCONFIG_OUTPUTS ${LAOS_CONFIG_OUT_DIR}/${_header_name}_autoconf.h)
endforeach()

# Depend on config/.config and config/defconfig unconditionally so editing,
# creating, or deleting either always invalidates the generated headers,
# even if a file didn't exist at configure time. Also register them as
# configure-time dependencies so CMake reconfigures (and re-evaluates this
# file) whenever either changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${LAOS_DOTCONFIG} ${LAOS_DEFCONFIG})

add_custom_command(
    OUTPUT ${LAOS_KCONFIG_OUTPUTS}
    COMMAND ${Python3_EXECUTABLE} ${LAOS_GENCONFIG_SCRIPT}
            --kconfig ${LAOS_KCONFIG_ROOT}
            --dotconfig ${LAOS_DOTCONFIG}
            --defconfig ${LAOS_DEFCONFIG}
            --src-root ${CMAKE_SOURCE_DIR}
            --out-dir ${LAOS_CONFIG_OUT_DIR}
    DEPENDS
        ${LAOS_KCONFIG_ROOT}
        ${LAOS_SUBSYSTEM_KCONFIGS}
        ${LAOS_GENCONFIG_SCRIPT}
        ${LAOS_DOTCONFIG}
        ${LAOS_DEFCONFIG}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating per-subsystem Kconfig headers"
)

add_custom_target(kconfig_headers DEPENDS ${LAOS_KCONFIG_OUTPUTS})

# Give `<target>` access to its generated <subsystem>_autoconf.h and make
# sure it's (re)built first. Include `"kernel_fs_autoconf.h"` (etc.) rather
# than a bare `"autoconf.h"` in source: every subsystem's header lives in the
# same output directory with a unique, subsystem-qualified name, so a target
# that links several subsystems together never has one header shadow another.
function(laos_use_kconfig TARGET)
    add_dependencies(${TARGET} kconfig_headers)
    target_include_directories(${TARGET} PUBLIC ${LAOS_CONFIG_OUT_DIR})
endfunction()

# Interactive TUI to edit config/.config (requires `pip install kconfiglib`).
# KCONFIG_CONFIG tells kconfiglib's menuconfig where to read/write .config,
# instead of its default of the current working directory.
add_custom_target(menuconfig
    COMMAND ${CMAKE_COMMAND} -E env KCONFIG_CONFIG=${LAOS_DOTCONFIG} python3 -m menuconfig ${LAOS_KCONFIG_ROOT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
)

# Explicitly reset config/.config back to the committed config/defconfig
# baseline, overwriting any local customization. The seed rule above only
# ever does this when .config is entirely missing; use this target to do it
# on purpose.
add_custom_target(defconfig
    COMMAND ${CMAKE_COMMAND} -E copy ${LAOS_DEFCONFIG} ${LAOS_DOTCONFIG}
    COMMENT "Resetting config/.config to config/defconfig"
)

# Write the current .config back to config/defconfig, e.g. after tuning
# values with `menuconfig`, so the new baseline can be committed.
add_custom_target(savedefconfig
    COMMAND ${Python3_EXECUTABLE} -c "import os; os.environ['srctree']=os.path.abspath('.'); from kconfiglib import Kconfig; kconf = Kconfig('${LAOS_KCONFIG_ROOT}'); kconf.load_config('${LAOS_DOTCONFIG}') if os.path.exists('${LAOS_DOTCONFIG}') else None; kconf.write_config('${LAOS_DEFCONFIG}')"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Writing current configuration to ${LAOS_DEFCONFIG}"
)
