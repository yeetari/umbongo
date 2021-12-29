# Kernel options.
set(KERNEL_FONT "${CMAKE_SOURCE_DIR}/FantasqueSansMono.ttf" CACHE STRING "Font to use in the kernel")
set(KERNEL_FONT_SIZE "24" CACHE STRING "Font size to use in the kernel")
set(KERNEL_QEMU_DEBUG OFF CACHE BOOL "Enable debug logging to port E9 in the kernel")
set(KERNEL_STACK_PROTECTOR OFF CACHE BOOL "Enable stack smashing protector in the kernel")
set(KERNEL_UBSAN OFF CACHE BOOL "Enable UBSan in the kernel")

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
    acpi/PciTable.cc
    acpi/RootTable.cc
    acpi/Table.cc
    cpu/LocalApic.cc
    cpu/Processor.cc
    dev/DevFs.cc
    dev/Device.cc
    dev/FramebufferDevice.cc
    fs/FileHandle.cc
    fs/Inode.cc
    fs/InodeFile.cc
    fs/RamFs.cc
    fs/Vfs.cc
    intr/InterruptManager.cc
    intr/IoApic.cc
    ipc/DoubleBuffer.cc
    ipc/Pipe.cc
    ipc/ServerSocket.cc
    ipc/Socket.cc
    mem/Heap.cc
    mem/MemoryManager.cc
    mem/PhysicalPage.cc
    mem/Region.cc
    mem/VirtSpace.cc
    pci/Enumerate.cc
    pci/Function.cc
    proc/Process.cc
    proc/Scheduler.cc
    proc/Thread.cc
    proc/ThreadBlocker.cc
    time/Hpet.cc
    time/TimeManager.cc
    Console.cc
    Dmesg.cc
    ${CMAKE_CURRENT_BINARY_DIR}/Font.cc
    kmain.cc
    Syscalls.cc
    UbSan.cc)
add_library(kernel_asm OBJECT
    cpu/cpu.asm
    cpu/interrupts.asm
    cpu/smp.asm)
install(TARGETS kernel DESTINATION .)
target_compile_options(kernel PRIVATE
    $<$<BOOL:${KERNEL_UBSAN}>:-fsanitize=undefined -fno-sanitize=function>
    $<$<BOOL:${KERNEL_STACK_PROTECTOR}>:-fstack-protector-strong>
    $<$<BOOL:${KERNEL_STACK_PROTECTOR}>:-mstack-protector-guard=global>
    -fno-pic
    -mno-red-zone
    -mno-sse
    # Allow non-trivial globals in the kernel.
    -Wno-exit-time-destructors
    -Wno-global-constructors)
target_gc_sections(kernel)
target_link_libraries(kernel PRIVATE elf kernel_asm ustd-no-sse)
target_link_options(kernel PRIVATE LINKER:-T,${CMAKE_CURRENT_SOURCE_DIR}/linker.ld,-no-pie)
