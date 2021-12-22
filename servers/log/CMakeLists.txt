add_executable(log-server main.cc)
install(TARGETS log-server DESTINATION bin)
target_gc_sections(log-server)
target_link_libraries(log-server PRIVATE core ipc)
