add_library(ipc Channel.cc Endpoint.cc)
install(TARGETS ipc DESTINATION lib)
target_gc_sections(ipc)
target_link_libraries(ipc PUBLIC core)
