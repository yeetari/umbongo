add_library(ipc Client.cc)
install(TARGETS ipc DESTINATION lib)
target_gc_sections(ipc)
target_link_libraries(ipc PUBLIC core)
