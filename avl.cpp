
#include "avl.h"

#include <algorithm>

template <typename K, typename V>
void AvlTree<K, V>::insert(K key, V value) {
    root = insert(root, key, value);
    if (root) root->parent = nullptr;
}

template <typename K, typename V>
void AvlTree<K, V>::remove(K key) {
    root = remove(root, key);
    if (root) {
        root->parent = nullptr;
    }
}

template <typename K, typename V>
void AvlTree<K, V>::updateHeight(Node* node) {
    node->height = 1 + std::max(height(node->left), height(node->right));
}

template <typename K, typename V>
typename AvlTree<K, V>::Node* AvlTree<K, V>::rotateRight(Node* y) {
    Node* x = y->left;
    Node* T2 = x->right;

    x->right = y;
    y->left = T2;

    x->parent = y->parent;
    y->parent = x;
    if (T2) {
        T2->parent = y;
    }

    updateHeight(y);
    updateHeight(x);

    return x;
}

template <typename K, typename V>
typename AvlTree<K, V>::Node* AvlTree<K, V>::rotateLeft(Node* x) {
    Node* y = x->right;
    Node* T2 = y->left;

    y->left = x;
    x->right = T2;

    y->parent = x->parent;
    x->parent = y;
    if (T2) {
        T2->parent = x;
    }

    updateHeight(x);
    updateHeight(y);

    return y;
}

template <typename K, typename V>
typename AvlTree<K, V>::Node* AvlTree<K, V>::balance(Node* node) {
    int bf = balanceFactor(node);

    // Left heavy
    if (bf > 1) {
        if (balanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
            if (node->left) node->left->parent = node;
        }
        return rotateRight(node);
    }
    // Right heavy
    if (bf < -1) {
        if (balanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
            if (node->right) node->right->parent = node;
        }
        return rotateLeft(node);
    }

    return node;
}

template <typename K, typename V>
typename AvlTree<K, V>::Node* AvlTree<K, V>::insert(Node* node, K key, V value) {
    if (!node) {
        return new Node(key, value);
    }
    if (key < node->key) {
        node->left = insert(node->left, key, value);
        if (node->left) node->left->parent = node;
    } else if (key > node->key) {
        node->right = insert(node->right, key, value);
        if (node->right) node->right->parent = node;
    } else {
        // Duplicate key, update value
        node->value = value;
        return node;
    }

    updateHeight(node);
    return balance(node);
}

template <typename K, typename V>
typename AvlTree<K, V>::Node* AvlTree<K, V>::removeMin(Node* node) {
    if (!node->left) {
        if (node->right) {
            node->right->parent = node->parent;
        }
        return node->right;
    }
    node->left = removeMin(node->left);
    if (node->left) {
        node->left->parent = node;
    }
    updateHeight(node);
    return balance(node);
}

template <typename K, typename V>
typename AvlTree<K, V>::Node* AvlTree<K, V>::remove(Node* node, K key) {
    if (!node) {
        return nullptr;
    }

    if (key < node->key) {
        node->left = remove(node->left, key);
        if (node->left) {
            node->left->parent = node;
        }
    } else if (key > node->key) {
        node->right = remove(node->right, key);
        if (node->right) {
            node->right->parent = node;
        }
    } else {
        // Node with only one child or no child
        if (!node->left) {
            Node* temp = node->right;
            if (temp) {
                temp->parent = node->parent;
            }
            delete node;
            node = temp;
        } else if (!node->right) {
            Node* temp = node->left;
            if (temp) {
                temp->parent = node->parent;
            }
            delete node;
            node = temp;
        } else {
            // Node with two children
            Node* temp = findMin(node->right);
            node->key = temp->key;
            node->value = temp->value;
            node->right = remove(node->right, temp->key);
            if (node->right) {
                node->right->parent = node;
            }
        }
    }

    if (!node) {
        return node;
    }

    updateHeight(node);
    return balance(node);
}

template <typename K, typename V>
void AvlTree<K, V>::destroy(Node* node) {
    if (!node) {
        return;
    }

    destroy(node->left);
    destroy(node->right);
    delete node;
}