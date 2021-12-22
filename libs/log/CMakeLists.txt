add_library(log Log.cc)
install(TARGETS log DESTINATION lib)
target_gc_sections(log)
target_link_libraries(log PRIVATE ipc)
