#pragma once

#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace ustd {

template <typename>
class TreeNodeBase;

template <typename D>
concept TreeNode = is_base_of<TreeNodeBase<D>, D>;

template <typename D>
class TreeNodeBase {
    template <Integral K, TreeNode N, typename N::template key_fn_t<K> get_key>
    friend class RedBlackTree;

private:
    D *parent{nullptr};
    union {
        Array<D *, 2> children{};
        struct {
            D *left_child;
            D *right_child;
        };
    };
    enum Colour {
        Red,
        Black,
    } colour{Red};

public:
    template <Integral K>
    using key_fn_t = K (D::*)() const;
};

template <Integral K, TreeNode N, typename N::template key_fn_t<K> get_key>
class RedBlackTree {
    enum Direction {
        L,
        R,
    };

private:
    N *m_root_node{nullptr};
    N *m_minimum_node{nullptr};

    template <Direction Dir>
    void rotate(N *node);
    void insert_fixups(N *node);

public:
    void insert(N *node);
    void remove(N *node);

    N *root_node() const { return m_root_node; }
    N *minimum_node() const { return m_minimum_node; }
};

template <Integral K, TreeNode N, typename N::template key_fn_t<K> get_key>
template <typename RedBlackTree<K, N, get_key>::Direction Dir>
void RedBlackTree<K, N, get_key>::rotate(N *node) {
    auto *parent = node->parent;
    auto *pivot = node->children[1 - Dir];
    ASSERT(pivot != nullptr);

    auto *pivot_child = pivot->children[Dir];
    node->children[1 - Dir] = pivot_child;
    if (pivot_child != nullptr) {
        pivot_child->parent = node;
    }

    pivot->children[Dir] = node;
    node->parent = pivot;

    pivot->parent = parent;
    if (parent == nullptr) {
        m_root_node = pivot;
    } else if (parent->left_child == node) {
        parent->left_child = pivot;
    } else {
        parent->right_child = pivot;
    }
}

template <Integral K, TreeNode N, typename N::template key_fn_t<K> get_key>
void RedBlackTree<K, N, get_key>::insert_fixups(N *node) {
    while (node->parent != nullptr && node->parent->colour == N::Red) {
        auto *grand_parent = node->parent->parent;
        auto dir = grand_parent->right_child == node->parent ? L : R;
        auto *uncle = grand_parent->children[dir];
        if (uncle != nullptr && uncle->colour == N::Red) {
            node->parent->colour = N::Black;
            uncle->colour = N::Black;
            grand_parent->colour = N::Red;
            node = grand_parent;
        } else {
            if (node->parent->children[1 - dir] == node) {
                node = node->parent;
                dir == L ? rotate<R>(node) : rotate<L>(node);
            }
            node->parent->colour = N::Black;
            grand_parent->colour = N::Red;
            dir == L ? rotate<L>(grand_parent) : rotate<R>(grand_parent);
        }
    }
    m_root_node->colour = N::Black;
}

template <Integral K, TreeNode N, typename N::template key_fn_t<K> get_key>
void RedBlackTree<K, N, get_key>::insert(N *node) {
    N *parent = m_root_node;
    for (auto *temp = parent; temp != nullptr;) {
        if ((node->*get_key)() < (temp->*get_key)()) {
            temp = temp->left_child;
        } else {
            temp = temp->right_child;
        }
        if (temp != nullptr) {
            parent = temp;
        }
    }

    if (parent == nullptr) [[unlikely]] {
        // New root node.
        node->colour = N::Black;
        m_root_node = node;
        m_minimum_node = node;
        return;
    }

    node->parent = parent;
    if ((node->*get_key)() < (parent->*get_key)()) {
        ASSERT(parent->left_child == nullptr);
        parent->left_child = node;
    } else {
        ASSERT(parent->right_child == nullptr);
        parent->right_child = node;
    }

    if (node->parent->parent != nullptr) {
        insert_fixups(node);
    }

    if (m_minimum_node->left_child == node) {
        m_minimum_node = node;
    }
}

template <Integral K, TreeNode N, typename N::template key_fn_t<K> get_key>
void RedBlackTree<K, N, get_key>::remove(N *node) {}

class FooNode : public TreeNodeBase<FooNode> {
public:
    int key() const;
};

using FooTree = RedBlackTree<int, FooNode, &FooNode::key>;

inline void foo() {
    FooTree tree;
    tree.insert(nullptr);
}

} // namespace ustd
