#pragma once

#include "Ast.hh" // IWYU pragma: keep
#include "Token.hh"

#include <ustd/Optional.hh>
#include <ustd/UniquePtr.hh>

class Lexer;

class Parser {
    Lexer &m_lexer;

    Optional<Token> consume(TokenKind kind);
    Token expect(TokenKind kind);

    UniquePtr<ast::Node> parse_command();
    UniquePtr<ast::Node> parse_list();
    UniquePtr<ast::Node> parse_pipe();

public:
    explicit Parser(Lexer &lexer) : m_lexer(lexer) {}

    UniquePtr<ast::Node> parse();
};
