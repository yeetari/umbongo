add_library(core File.cc)
target_gc_sections(core)
target_link_libraries(core PRIVATE ustd)
