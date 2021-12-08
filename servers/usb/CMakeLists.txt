add_executable(usb-server main.cc)
install(TARGETS usb-server DESTINATION bin)
target_gc_sections(usb-server)
target_link_libraries(usb-server PRIVATE core)
