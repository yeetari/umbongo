add_executable(root-server main.cc)
install(TARGETS root-server DESTINATION bin)
target_gc_sections(root-server)
target_link_libraries(root-server core ipc)
