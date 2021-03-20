# Kernel options.
set(KERNEL_FONT "${CMAKE_SOURCE_DIR}/FantasqueSansMono.ttf" CACHE STRING "Font to use in the kernel")
set(KERNEL_FONT_SIZE "24" CACHE STRING "Font size to use in the kernel")
set(KERNEL_QEMU_DEBUG OFF CACHE BOOL "Enable debug logging to port E9 in the kernel")
set(KERNEL_STACK_PROTECTOR OFF CACHE BOOL "Enable stack smashing protector in the kernel")

# Generate kernel config header.
configure_file(Config.hh.in Config.hh)
file(GENERATE OUTPUT Config.hh INPUT ${CMAKE_CURRENT_BINARY_DIR}/Config.hh)

# Generate font at build time.
add_custom_command(OUTPUT Font.cc
    COMMAND ${CMAKE_BINARY_DIR}/tools/font-generator ${KERNEL_FONT} ${CMAKE_CURRENT_BINARY_DIR}/Font.cc ${KERNEL_FONT_SIZE}
    DEPENDS font-generator)

# Kernel target.
add_executable(kernel
    acpi/ApicTable.cc
    acpi/RootTable.cc
    acpi/Table.cc
    cpu/LocalApic.cc
    cpu/Processor.cc
    intr/InterruptManager.cc
    intr/IoApic.cc
    mem/MemoryManager.cc
    mem/VirtSpace.cc
    proc/Process.cc
    proc/Scheduler.cc
    Console.cc
    ${CMAKE_CURRENT_BINARY_DIR}/Font.cc
    kmain.cc
    Syscalls.cc)
add_library(kernel_asm OBJECT
    cpu/cpu.asm
    cpu/interrupts.asm)
install(TARGETS kernel DESTINATION .)
target_compile_features(kernel PRIVATE cxx_std_20)
target_compile_options(kernel PRIVATE
    -ffreestanding -fno-exceptions -fno-rtti
    $<$<BOOL:${KERNEL_STACK_PROTECTOR}>:-fstack-protector-strong>
    $<$<BOOL:${KERNEL_STACK_PROTECTOR}>:-mstack-protector-guard=global>
    -mno-red-zone
    -nostdinc++ -nostdlib)
target_gc_sections(kernel)
target_link_libraries(kernel PRIVATE kernel_asm ustd)
target_link_options(kernel PRIVATE LINKER:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostdlib)
