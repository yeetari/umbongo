#pragma once

#include "Token.hh"

#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

class Lexer {
    ustd::String m_source;
    usize m_source_position{0};
    Token m_peek_token{TokenKind::Eof};
    bool m_peek_ready{false};

    Token next_token();

public:
    explicit Lexer(ustd::String &&source) : m_source(ustd::move(source)) {}

    Token next();
    const Token &peek();
};
