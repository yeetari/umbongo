cmake_minimum_required(VERSION 3.18)
# TODO: Use presets for this.
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE toolchain/x86_64-pc-none-clang.cmake)
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
set_property(CACHE BUILD_TYPE PROPERTY STRINGS Debug Release)

# Configure cmake options from global options.
set(CMAKE_BUILD_TYPE ${BUILD_TYPE} CACHE INTERNAL "" FORCE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${LINK_TIME_OPT} CACHE INTERNAL "" FORCE)
set(CMAKE_INSTALL_MESSAGE NEVER)
set(CMAKE_INSTALL_PREFIX sysroot)

# Enable PIC.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")

# Use C++ 20 globally.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use -O2 instead of -O3.
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# Enable GC sections and stripping for release builds.
function(target_gc_sections target)
    if(BUILD_TYPE STREQUAL "Release")
        target_compile_options(${target} PRIVATE -fdata-sections -ffunction-sections)
        target_link_options(${target} PRIVATE LINKER:--gc-sections,-s)
    endif()
endfunction()

# Enable warnings globally.
# TODO: This probably shouldn't be here, maybe cmake presets could be used instead?
if(BUILD_WARNINGS)
    add_compile_options(
        -Werror
        -Weverything
        -Wno-c++20-compat
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-cast-function-type
        -Wno-covered-switch-default
        -Wno-gnu-anonymous-struct
        -Wno-gnu-statement-expression
        -Wno-gnu-zero-variadic-macro-arguments
        -Wno-missing-noreturn
        -Wno-missing-prototypes
        -Wno-missing-variable-declarations
        -Wno-nested-anon-types
        -Wno-padded
        -Wno-reserved-identifier
        -Wno-shadow-uncaptured-local
        -Wno-switch-enum
        -Wno-unknown-warning-option
        -Wno-unused-member-function
        -Wno-unused-private-field
        -Wno-weak-vtables
        -Wno-zero-length-array)
endif()

find_program(MCOPY_EXECUTABLE mcopy)
find_program(MFORMAT_EXECUTABLE mformat)
if(MCOPY_EXECUTABLE AND MFORMAT_EXECUTABLE)
    add_custom_target(image
        BYPRODUCTS fs.img
        COMMAND truncate --size 28800000 fs.img
        COMMAND ${MFORMAT_EXECUTABLE} -i fs.img -f 2880 -h 20 ::
        COMMAND ${MCOPY_EXECUTABLE} -si fs.img ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX}/* ::
        DEPENDS install)
else()
    message(WARNING "Failed to find mtools, image target will be unavailable!")
endif()

find_program(QEMU_EXECUTABLE qemu-system-x86_64)
if(QEMU_EXECUTABLE)
    if(EXISTS /dev/kvm)
        set(KVM_AVAILABLE YES)
    endif()
    find_file(OVMF_BINARY
        NAMES OVMF.fd OVMF_CODE.fd
        PATHS ${CMAKE_SOURCE_DIR} /usr/share/ovmf /usr/share/edk2-ovmf)
    if(OVMF_BINARY)
        set(QEMU_FLAGS ${QEMU_FLAGS}
            -bios ${OVMF_BINARY}
            -cpu max
            -debugcon stdio
            -device qemu-xhci
            -device usb-kbd
            -device usb-storage,drive=esp
            -device virtio-vga,xres=1920,yres=1080
            -display sdl
            -drive file=fat:rw:${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX},format=raw,id=esp,if=none
            -machine q35
            -nodefaults
            -no-reboot
            -smp 4)
        add_custom_target(run
            COMMAND ${QEMU_EXECUTABLE} ${QEMU_FLAGS} $<$<BOOL:${KVM_AVAILABLE}>:-enable-kvm>
            DEPENDS install
            USES_TERMINAL)
        add_custom_target(run-no-kvm
            COMMAND ${QEMU_EXECUTABLE} ${QEMU_FLAGS}
            DEPENDS install
            USES_TERMINAL)
    else()
        message(WARNING "Failed to find OVMF, run target will be unavailable!")
    endif()
else()
    message(WARNING "Failed to find qemu, run target will be unavailable!")
endif()

# Build host targets first so they aren't affected by the upcoming options.
add_subdirectory(tools)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -nostdinc -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffreestanding -fno-exceptions -fno-rtti -nostdinc -nostdinc++ -nostdlib")
add_link_options(-pie -nostdlib -Wno-unused-command-line-argument)
include_directories(${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} libs)

add_subdirectory(boot)
add_subdirectory(dynamic-linker)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/${CMAKE_INSTALL_PREFIX} DESTINATION ${CMAKE_BINARY_DIR})

add_link_options(LINKER:-dynamic-linker,/bin/dynamic-linker)
add_subdirectory(coreutils)
add_subdirectory(kernel)
add_subdirectory(libs)
add_subdirectory(servers)
add_subdirectory(shell)
add_subdirectory(ustd)
