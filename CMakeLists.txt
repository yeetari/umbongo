cmake_minimum_required(VERSION 3.16)
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE toolchain/clang.cmake)
endif()
project(umbongo ASM_NASM C CXX)

# Hide some options.
mark_as_advanced(CMAKE_INSTALL_PREFIX)
mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

# Global options for the whole system.
set(BUILD_TYPE "Debug" CACHE STRING "Build with optimisations")
set(BUILD_WARNINGS OFF CACHE BOOL "Build with warnings")
set(ENABLE_ASSERTIONS ON CACHE BOOL "Enable assertions")
set(ENABLE_ASSERTIONS_PEDANTIC OFF CACHE BOOL "Enable pedantic assertions")
set(LINK_TIME_OPT OFF CACHE BOOL "Build with LTO")
set(OVMF_BINARY "${CMAKE_SOURCE_DIR}/OVMF.fd" CACHE STRING "OVMF binary for emulation")
set_property(CACHE BUILD_TYPE PROPERTY STRINGS Debug Release)

# Configure cmake options from global options.
set(CMAKE_BUILD_TYPE ${BUILD_TYPE} CACHE INTERNAL "" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${LINK_TIME_OPT} CACHE INTERNAL "" FORCE)
set(CMAKE_INSTALL_PREFIX sysroot)

# Disable SSE.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-sse")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-sse")

# Use -O2 instead of -O3.
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# Enable GC sections and stripping for release builds.
function(target_gc_sections target)
    if(BUILD_TYPE STREQUAL "Release")
        target_compile_options(${target} PRIVATE -fdata-sections -ffunction-sections)
        target_link_options(${target} PRIVATE -Wl,--gc-sections,-s)
    endif()
endfunction()

# Enable warnings globally.
# TODO: This probably shouldn't be here, maybe cmake profiles could be used instead?
if(BUILD_WARNINGS)
    add_compile_options(
        -Werror
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-gnu-zero-variadic-macro-arguments
        -Wno-missing-noreturn
        -Wno-missing-prototypes
        -Wno-missing-variable-declarations
        -Wno-padded
        -Wno-switch-enum
        -Wno-unused-member-function
        -Wno-unused-private-field)
endif()

add_custom_target(image
    BYPRODUCTS fs.img
    # Can't depend on install target, call it manually instead.
    COMMAND ${CMAKE_COMMAND} --build . --target install
    COMMAND dd if=/dev/zero of=fs.img bs=1k count=1440
    COMMAND mformat -i fs.img -f 1440 ::
    COMMAND mcopy -si fs.img ${CMAKE_INSTALL_PREFIX}/* ::
    USES_TERMINAL)

find_program(QEMU_PROG NAMES qemu-system-x86_64)
if(QEMU_PROG)
    set(QEMU_FLAGS ${QEMU_FLAGS}
        -bios ${OVMF_BINARY}
        -cpu max
        -debugcon stdio
        -drive file=fs.img,if=ide,format=raw
        -nodefaults
        -vga std)
    add_custom_target(run
        COMMAND ${QEMU_PROG} ${QEMU_FLAGS} -enable-kvm
        DEPENDS image
        USES_TERMINAL)
    add_custom_target(run-no-kvm
        COMMAND ${QEMU_PROG} ${QEMU_FLAGS}
        DEPENDS image
        USES_TERMINAL)
else()
    message(WARNING "Qemu not found, run target will be unavailable!")
endif()

include_directories(${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR})
add_subdirectory(boot)
add_subdirectory(init)
add_subdirectory(kernel)
add_subdirectory(tools)
add_subdirectory(ustd)
