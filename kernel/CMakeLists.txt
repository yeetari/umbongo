add_executable(kernel kmain.cc)
install(TARGETS kernel DESTINATION .)
target_compile_features(kernel PRIVATE cxx_std_20)
target_compile_options(kernel PRIVATE
    -ffreestanding -fno-exceptions -fno-rtti
    -mno-red-zone
    -nostdinc++ -nostdlib)
target_link_options(kernel PRIVATE LINKER:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostdlib)
