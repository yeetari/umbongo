add_executable(boot boot.cc)
install(TARGETS boot DESTINATION EFI/BOOT)
set_target_properties(boot PROPERTIES OUTPUT_NAME "BOOTX64.EFI")
target_compile_features(boot PRIVATE cxx_std_20)
target_compile_options(boot PRIVATE
    -ffreestanding -fno-exceptions -fno-rtti -fshort-wchar
    -mno-red-zone
    -nostdinc++ -nostdlib
    -target x86_64-unknown-windows)
target_link_libraries(boot PRIVATE ustd)
target_link_options(boot PRIVATE LINKER:
    -nostdlib
    -target x86_64-unknown-windows
    -Wl,-entry:efi_main
    -Wl,-subsystem:efi_application)
