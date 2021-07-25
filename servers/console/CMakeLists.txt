add_executable(console-server main.cc)
install(TARGETS console-server DESTINATION .)
target_gc_sections(console-server)
target_link_libraries(console-server PRIVATE core)
