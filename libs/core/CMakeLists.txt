add_library(core
    Error.cc
    Handle.cc
    Heap.cc
    Process.cc
    start.cc
    Time.cc)
install(TARGETS core DESTINATION lib)
target_gc_sections(core)
target_link_libraries(core PUBLIC ustd)
