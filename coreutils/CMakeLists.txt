function(umbongo_utility name)
    add_executable(${name} ${name}.cc)
    install(TARGETS ${name} DESTINATION bin)
    target_gc_sections(${name})
    target_link_libraries(${name} PRIVATE core ${ARGN})
endfunction()

umbongo_utility(cat)
umbongo_utility(yes)
