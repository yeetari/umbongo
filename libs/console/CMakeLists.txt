add_library(console Console.cc)
install(TARGETS console DESTINATION lib)
target_gc_sections(console)
target_link_libraries(console PRIVATE ipc)
