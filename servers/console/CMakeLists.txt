# Generate font at build time.
add_custom_command(OUTPUT Font.cc
    COMMAND ${CMAKE_BINARY_DIR}/tools/font-generator ${KERNEL_FONT} ${CMAKE_CURRENT_BINARY_DIR}/Font.cc ${KERNEL_FONT_SIZE}
    DEPENDS font-generator)

add_executable(console-server main.cc ${CMAKE_CURRENT_BINARY_DIR}/Font.cc)
install(TARGETS console-server DESTINATION .)
target_gc_sections(console-server)
target_link_libraries(console-server PRIVATE core)
