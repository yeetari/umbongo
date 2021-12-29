add_executable(usb-server
    Device.cc
    Endpoint.cc
    HostController.cc
    KeyboardDevice.cc
    main.cc
    Port.cc
    TrbRing.cc)
install(TARGETS usb-server DESTINATION bin)
target_gc_sections(usb-server)
target_link_libraries(usb-server PRIVATE config core mmio)
