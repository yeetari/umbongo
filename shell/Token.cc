#include "Token.hh"

#include <ustd/Assert.hh>
#include <ustd/String.hh>

const char *Token::kind_string(TokenKind kind) {
    switch (kind) {
    case TokenKind::Eof:
        return "eof";
    case TokenKind::Identifier:
        return "identifier";
    case TokenKind::Pipe:
        return "|";
    }
}

const ustd::String &Token::text() const {
    ASSERT(m_kind == TokenKind::Identifier);
    return m_text;
}

const char *Token::to_string() const {
    switch (m_kind) {
    case TokenKind::Identifier:
        return m_text.data();
    default:
        return kind_string(m_kind);
    }
}
