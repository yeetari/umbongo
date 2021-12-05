#pragma once

#include "Token.hh"

#include <ustd/StringView.hh>
#include <ustd/Types.hh>

class Lexer {
    ustd::StringView m_source;
    usize m_source_position{0};
    Token m_peek_token{TokenKind::Eof};
    bool m_peek_ready{false};

    Token next_token();

public:
    explicit Lexer(ustd::StringView source) : m_source(source) {}

    Token next();
    const Token &peek();
};
