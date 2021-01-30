# Kernel options.
set(KERNEL_STACK_PROTECTOR OFF CACHE BOOL "Enable stack smashing protector in the kernel")
set(KERNEL_QEMU_DEBUG OFF CACHE BOOL "Enable debug logging to port E9 in the kernel")

# Generate kernel config header.
configure_file(Config.hh.in Config.hh)
file(GENERATE OUTPUT Config.hh INPUT ${CMAKE_CURRENT_BINARY_DIR}/Config.hh)

# Kernel target.
add_executable(kernel kmain.cc)
install(TARGETS kernel DESTINATION .)
target_compile_features(kernel PRIVATE cxx_std_20)
target_compile_options(kernel PRIVATE
    -ffreestanding -fno-exceptions -fno-rtti
    $<$<BOOL:${KERNEL_STACK_PROTECTOR}>:-fstack-protector-all>
    -mno-red-zone
    -nostdinc++ -nostdlib)
target_link_libraries(kernel PRIVATE ustd)
target_link_options(kernel PRIVATE LINKER:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostdlib)
