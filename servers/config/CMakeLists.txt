add_executable(config-server main.cc)
install(TARGETS config-server DESTINATION bin)
target_gc_sections(config-server)
target_link_libraries(config-server PRIVATE core ipc)
