add_library(core
    Directory.cc
    Error.cc
    EventLoop.cc
    File.cc
    FileSystem.cc
    Heap.cc
    Pipe.cc
    Process.cc
    start.cc
    Time.cc
    Timer.cc)
install(TARGETS core DESTINATION lib)
target_gc_sections(core)
target_link_libraries(core PUBLIC log ustd)
