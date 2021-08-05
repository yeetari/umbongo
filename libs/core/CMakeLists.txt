add_library(core Error.cc File.cc Heap.cc Log.cc Process.cc start.cc)
target_gc_sections(core)
target_link_libraries(core PUBLIC ustd)
