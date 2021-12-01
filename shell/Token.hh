#pragma once

#include <ustd/String.hh>
#include <ustd/Utility.hh>

enum class TokenKind {
    Eof,
    Identifier,
    Pipe,
};

class Token {
    TokenKind m_kind;
    ustd::String m_text;

public:
    static const char *kind_string(TokenKind kind);

    Token(TokenKind kind) : m_kind(kind) {}
    Token(ustd::String &&text) : m_kind(TokenKind::Identifier), m_text(ustd::move(text)) {}

    TokenKind kind() const { return m_kind; }
    const ustd::String &text() const;
    const char *to_string() const;
};
