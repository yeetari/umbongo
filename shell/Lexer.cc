#include "Lexer.hh"
#include "Token.hh"

#include <ustd/Log.hh>
#include <ustd/String.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

bool is_alpha(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

bool is_space(char ch) {
    return ch == ' ' || ch == '\f' || ch == '\n' || ch == '\r' || ch == '\t' || ch == '\v';
}

bool is_identifier_character(char ch) {
    return is_alpha(ch) || ch == '_' || ch == '/';
}

} // namespace

Token Lexer::next_token() {
    if (m_source_position == m_source.length()) {
        return TokenKind::Eof;
    }
    while (is_space(m_source[m_source_position])) {
        m_source_position++;
    }

    char ch = m_source[m_source_position++];
    if (ch == '|') {
        return TokenKind::Pipe;
    }
    if (is_identifier_character(ch)) {
        Vector<char> buf;
        buf.push(ch);
        while (m_source_position != m_source.length() && is_identifier_character(m_source[m_source_position])) {
            buf.push(m_source[m_source_position++]);
        }
        return String(buf.data(), buf.size());
    }
    logln("ush: unexpected {:c}", ch);
    return TokenKind::Eof;
}

Token Lexer::next() {
    if (m_peek_ready) {
        m_peek_ready = false;
        return ustd::move(m_peek_token);
    }
    return next_token();
}

const Token &Lexer::peek() {
    if (!m_peek_ready) {
        m_peek_ready = true;
        m_peek_token = next_token();
    }
    return m_peek_token;
}
