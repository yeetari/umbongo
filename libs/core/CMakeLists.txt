add_library(core File.cc Heap.cc Log.cc start.cc)
target_gc_sections(core)
target_link_libraries(core PUBLIC ustd)
