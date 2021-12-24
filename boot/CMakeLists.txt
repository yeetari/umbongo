add_executable(boot boot.cc ../ustd/Memory.cc)
install(TARGETS boot DESTINATION EFI/BOOT)
set_target_properties(boot PROPERTIES OUTPUT_NAME "BOOTX64.EFI")
target_compile_options(boot PRIVATE
    -fno-pic -fshort-wchar
    -mno-red-zone
    -Wno-unused-command-line-argument
    -target x86_64-unknown-windows)
target_link_options(boot PRIVATE LINKER:
    -target x86_64-unknown-windows
    -Wl,-entry:efi_main
    -Wl,-subsystem:efi_application)
