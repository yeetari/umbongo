add_executable(init init.cc)
install(TARGETS init DESTINATION .)
target_compile_features(init PRIVATE cxx_std_20)
target_compile_options(init PRIVATE
    -ffreestanding -fno-exceptions -fno-rtti
    -fPIE
    -nostdinc++ -nostdlib)
target_gc_sections(init)
target_link_options(init PRIVATE LINKER:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -nostdlib -pie)
