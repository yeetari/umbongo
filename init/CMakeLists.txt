add_executable(init init.cc)
install(TARGETS init DESTINATION .)
target_gc_sections(init)
target_link_libraries(init PRIVATE core ustd)
