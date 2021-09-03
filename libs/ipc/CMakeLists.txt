add_library(ipc Client.cc Server.cc)
target_gc_sections(ipc)
target_link_libraries(ipc PUBLIC core)
