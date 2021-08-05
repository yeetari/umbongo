#pragma once

#include "Value.hh" // IWYU pragma: keep

#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace ast {

class Node {
public:
    Node() = default;
    Node(const Node &) = delete;
    Node(Node &&) = delete;
    virtual ~Node() = default;

    Node &operator=(const Node &) = delete;
    Node &operator=(Node &&) = delete;

    virtual void dump(usize indent) const;
    virtual UniquePtr<Value> evaluate() const = 0;
};

class Command : public Node {
    const UniquePtr<Node> m_node;

public:
    explicit Command(UniquePtr<Node> &&node) : m_node(ustd::move(node)) {}

    void dump(usize indent) const override;
    UniquePtr<Value> evaluate() const override;
};

class List : public Node {
    const Vector<UniquePtr<Node>> m_nodes;

public:
    explicit List(Vector<UniquePtr<Node>> &&nodes) : m_nodes(ustd::move(nodes)) {}

    void dump(usize indent) const override;
    UniquePtr<Value> evaluate() const override;
};

class Pipe : public Node {
    const UniquePtr<Node> m_lhs;
    const UniquePtr<Node> m_rhs;

public:
    Pipe(UniquePtr<Node> &&lhs, UniquePtr<Node> &&rhs) : m_lhs(ustd::move(lhs)), m_rhs(ustd::move(rhs)) {}

    void dump(usize indent) const override;
    UniquePtr<Value> evaluate() const override;
};

class StringLiteral : public Node {
    const String m_text;

public:
    explicit StringLiteral(String text) : m_text(ustd::move(text)) {}

    void dump(usize indent) const override;
    UniquePtr<Value> evaluate() const override;
};

} // namespace ast
