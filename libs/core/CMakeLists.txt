add_library(core
    Error.cc
    EventLoop.cc
    File.cc
    FileSystem.cc
    Heap.cc
    Log.cc
    Pipe.cc
    Process.cc
    start.cc
    Timer.cc)
install(TARGETS core DESTINATION lib)
target_gc_sections(core)
target_link_libraries(core PUBLIC ustd)
