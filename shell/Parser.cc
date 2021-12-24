#include "Parser.hh"

#include "Ast.hh"
#include "Lexer.hh"
#include "Token.hh"

#include <core/Print.hh>
#include <ustd/Optional.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

ustd::Optional<Token> Parser::consume(TokenKind kind) {
    if (m_lexer.peek().kind() == kind) {
        return m_lexer.next();
    }
    return {};
}

Token Parser::expect(TokenKind kind) {
    Token next = m_lexer.next();
    if (next.kind() != kind) {
        core::println("ush: expected {} but got {}", Token::kind_string(kind), next.to_string());
        return TokenKind::Eof;
    }
    return next;
}

ustd::UniquePtr<ast::Node> Parser::parse_command() {
    return ustd::make_unique<ast::Command>(parse_list());
}

ustd::UniquePtr<ast::Node> Parser::parse_list() {
    ustd::Vector<ustd::UniquePtr<ast::Node>> nodes;
    nodes.push(ustd::make_unique<ast::StringLiteral>(expect(TokenKind::Identifier).text()));
    while (auto identifier = consume(TokenKind::Identifier)) {
        nodes.push(ustd::make_unique<ast::StringLiteral>(identifier->text()));
    }
    return ustd::make_unique<ast::List>(ustd::move(nodes));
}

ustd::UniquePtr<ast::Node> Parser::parse_pipe() {
    auto lhs = parse_command();
    if (!consume(TokenKind::Pipe)) {
        return lhs;
    }
    return ustd::make_unique<ast::Pipe>(ustd::move(lhs), parse_pipe());
}

ustd::UniquePtr<ast::Node> Parser::parse() {
    auto node = parse_pipe();
    if (auto token = m_lexer.next(); token.kind() != TokenKind::Eof) {
        core::println("ush: unexpected {}", token.to_string());
    }
    return node;
}
