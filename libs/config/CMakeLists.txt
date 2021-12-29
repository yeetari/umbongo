add_library(config Config.cc)
install(TARGETS config DESTINATION lib)
target_gc_sections(config)
target_link_libraries(config PRIVATE ipc)
