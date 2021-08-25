add_library(posix
    sys/stat.cc
    sys/wait.cc
    assert.cc
    ctype.cc
    fcntl.cc
    init.cc
    locale.cc
    setjmp.cc
    signal.cc
    stdio.cc
    stdlib.cc
    string.cc
    time.cc
    unistd.cc
    utime.cc)
install(DIRECTORY . DESTINATION include FILES_MATCHING PATTERN *.h)
install(TARGETS posix DESTINATION lib)
target_compile_options(posix PRIVATE -Wno-reserved-id-macro)
target_gc_sections(posix)
target_include_directories(posix PUBLIC .)
target_link_libraries(posix PRIVATE ustd)
