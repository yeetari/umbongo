add_executable(system-server main.cc)
install(TARGETS system-server DESTINATION .)
target_gc_sections(system-server)
target_link_libraries(system-server PRIVATE core)
