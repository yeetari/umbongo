add_executable(shell Ast.cc Lexer.cc main.cc Parser.cc Token.cc)
install(TARGETS shell DESTINATION .)
target_gc_sections(shell)
target_link_libraries(shell PRIVATE core)
