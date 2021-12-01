#pragma once

#include "Ast.hh" // IWYU pragma: keep
#include "Token.hh"

#include <ustd/Optional.hh>
#include <ustd/UniquePtr.hh>

class Lexer;

class Parser {
    Lexer &m_lexer;

    ustd::Optional<Token> consume(TokenKind kind);
    Token expect(TokenKind kind);

    ustd::UniquePtr<ast::Node> parse_command();
    ustd::UniquePtr<ast::Node> parse_list();
    ustd::UniquePtr<ast::Node> parse_pipe();

public:
    explicit Parser(Lexer &lexer) : m_lexer(lexer) {}

    ustd::UniquePtr<ast::Node> parse();
};
