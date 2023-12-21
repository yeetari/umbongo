#pragma once

#include "token.hh"

#include <ustd/string_view.hh>
#include <ustd/types.hh>

class Lexer {
    ustd::StringView m_source;
    size_t m_source_position{0};
    Token m_peek_token{TokenKind::Eof};
    bool m_peek_ready{false};

    Token next_token();

public:
    explicit Lexer(ustd::StringView source) : m_source(source) {}

    Token next();
    const Token &peek();
};
