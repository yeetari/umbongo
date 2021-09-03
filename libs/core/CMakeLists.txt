add_library(core Error.cc EventLoop.cc File.cc Heap.cc Log.cc Process.cc start.cc)
install(TARGETS core DESTINATION lib)
target_gc_sections(core)
target_link_libraries(core PUBLIC ustd)
