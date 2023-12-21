#pragma once

#include "ast.hh" // IWYU pragma: keep
#include "token.hh"

#include <ustd/optional.hh>
#include <ustd/unique_ptr.hh>

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
