#pragma once

#include <ustd/array.hh>
#include <ustd/numeric.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace ustd {

// TODO: More comments.

template <typename N>
class IntrusiveTreeNode;

template <typename N>
concept IntrusiveNode = is_base_of<IntrusiveTreeNode<N>, N>;

template <typename N>
class IntrusiveTreeNode {
public:
    // TODO: Make this not public somehow.
    template <Integral K>
    using GetKeyFn = K (N::*)() const;

private:
    template <Integral K, IntrusiveNode N1, typename N1::template GetKeyFn<K> get_key>
    friend class SplayTree;

    N *parent{nullptr};
    union {
        Array<N *, 2> children{};
        struct {
            N *left_child;
            N *right_child;
        };
    };
};

template <Integral K, IntrusiveNode N, typename N::template GetKeyFn<K> get_key>
class SplayTree {
    enum Direction {
        L,
        R,
    };

    template <Direction Dir>
    void rotate(N *node);
    void splay(N *node);

    N *m_root_node{nullptr};
    N *m_minimum_node{nullptr};

public:
    static N *successor(N *node);
    N *find_no_splay(K key) const;
    N *find(K key);
    void insert(N *node);
    void remove(N *node);

    N *root_node() const { return m_root_node; }
    N *minimum_node() const { return m_minimum_node; }
};

template <Integral K, IntrusiveNode N, typename N::template GetKeyFn<K> get_key>
N *SplayTree<K, N, get_key>::successor(N *node) {
    if (node->right_child != nullptr) {
        node = node->right_child;
        while (node->left_child != nullptr) {
            node = node->left_child;
        }
        return node;
    }
    auto *temp = node->parent;
    while (temp != nullptr && node == temp->right_child) {
        node = temp;
        temp = temp->parent;
    }
    return temp;
}

template <Integral K, IntrusiveNode N, typename N::template GetKeyFn<K> get_key>
template <typename SplayTree<K, N, get_key>::Direction Dir>
void SplayTree<K, N, get_key>::rotate(N *node) {
    auto *parent = node->parent;
    auto *pivot = node->children[1 - Dir];
    ASSERT(pivot != nullptr);

    auto *pivot_child = pivot->children[Dir];
    node->children[1 - Dir] = pivot_child;
    if (pivot_child != nullptr) {
        pivot_child->parent = node;
    }

    pivot->parent = parent;
    if (parent == nullptr) {
        m_root_node = pivot;
    } else if (parent->left_child == node) {
        parent->left_child = pivot;
    } else {
        parent->right_child = pivot;
    }
    pivot->children[Dir] = node;
    node->parent = pivot;
}

template <Integral K, IntrusiveNode N, typename N::template GetKeyFn<K> get_key>
void SplayTree<K, N, get_key>::splay(N *node) {
    while (auto *&parent = node->parent) {
        if (parent->parent == nullptr) {
            if (parent->left_child == node) {
                rotate<R>(parent);
            } else {
                rotate<L>(parent);
            }
        } else if (parent->left_child == node && parent->parent->left_child == parent) {
            rotate<R>(parent->parent);
            rotate<R>(parent);
        } else if (parent->right_child == node && parent->parent->right_child == parent) {
            rotate<L>(parent->parent);
            rotate<L>(parent);
        } else if (parent->left_child == node && parent->parent->right_child == parent) {
            rotate<R>(parent);
            rotate<L>(parent);
        } else {
            rotate<L>(parent);
            rotate<R>(parent);
        }
    }
    ASSERT(m_root_node == node);
}

template <Integral K, IntrusiveNode N, typename N::template GetKeyFn<K> get_key>
N *SplayTree<K, N, get_key>::find_no_splay(K key) const {
    auto *node = m_root_node;
    while (node != nullptr && (node->*get_key)() != key) {
        if (key < (node->*get_key)()) {
            node = node->left_child;
        } else {
            node = node->right_child;
        }
    }
    return node;
}

template <Integral K, IntrusiveNode N, typename N::template GetKeyFn<K> get_key>
N *SplayTree<K, N, get_key>::find(K key) {
    auto *node = find_no_splay(key);
    if (node != nullptr) {
        splay(node);
    }
    return node;
}

template <Integral K, IntrusiveNode N, typename N::template GetKeyFn<K> get_key>
void SplayTree<K, N, get_key>::insert(N *node) {
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
    splay(node);

    if (m_minimum_node->left_child == node) {
        m_minimum_node = node;
    }
}

template <Integral K, IntrusiveNode N, typename N::template GetKeyFn<K> get_key>
void SplayTree<K, N, get_key>::remove(N *node) {
    splay(node);

    if (m_minimum_node == node) {
        ASSERT(m_minimum_node->left_child == nullptr);
        m_minimum_node = successor(node);
    }

    if (auto *left = node->left_child) {
        m_root_node = left;
        if (node->right_child != nullptr) {
            while (left->right_child != nullptr) {
                left = left->right_child;
            }
            left->right_child = node->right_child;
            node->right_child->parent = left;
        }
    } else {
        m_root_node = node->right_child;
    }

    if (m_root_node != nullptr) {
        m_root_node->parent = nullptr;
    } else {
        ASSERT(m_minimum_node == nullptr);
    }
}

} // namespace ustd
