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

    virtual void dump(size_t indent) const;
    virtual ustd::UniquePtr<Value> evaluate() const = 0;
};

class Command : public Node {
    const ustd::UniquePtr<Node> m_node;

public:
    explicit Command(ustd::UniquePtr<Node> &&node) : m_node(ustd::move(node)) {}

    void dump(size_t indent) const override;
    ustd::UniquePtr<Value> evaluate() const override;
};

class List : public Node {
    const ustd::Vector<ustd::UniquePtr<Node>> m_nodes;

public:
    explicit List(ustd::Vector<ustd::UniquePtr<Node>> &&nodes) : m_nodes(ustd::move(nodes)) {}

    void dump(size_t indent) const override;
    ustd::UniquePtr<Value> evaluate() const override;
};

class Pipe : public Node {
    const ustd::UniquePtr<Node> m_lhs;
    const ustd::UniquePtr<Node> m_rhs;

public:
    Pipe(ustd::UniquePtr<Node> &&lhs, ustd::UniquePtr<Node> &&rhs) : m_lhs(ustd::move(lhs)), m_rhs(ustd::move(rhs)) {}

    void dump(size_t indent) const override;
    ustd::UniquePtr<Value> evaluate() const override;
};

class StringLiteral : public Node {
    const ustd::String m_text;

public:
    explicit StringLiteral(ustd::String text) : m_text(ustd::move(text)) {}

    void dump(size_t indent) const override;
    ustd::UniquePtr<Value> evaluate() const override;
};

} // namespace ast
