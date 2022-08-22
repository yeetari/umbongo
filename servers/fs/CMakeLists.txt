add_executable(fs-server
    main.cc
    Ufs.cc)
install(TARGETS fs-server DESTINATION bin)
target_gc_sections(fs-server)
target_link_libraries(fs-server PRIVATE core)
