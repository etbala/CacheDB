
#pragma once

template <typename K, typename V>
class AvlTree {
private:
    class Node {
    public:
        K key;
        V value;
        int height;
        Node* left;
        Node* right;
        Node* parent;

        Node(K k, V v) : key(k), value(v), height(1), 
                left(nullptr), right(nullptr), parent(nullptr) {}
    };

public:
    AvlTree() : root(nullptr) {}
    ~AvlTree() { destroy(root); }

    void insert(K key, V value);
    void remove(K key);

private:
    int height(Node* node) { return node ? node->height : 0; }
    int balanceFactor(Node* node) { return node ? height(node->left) - height(node->right) : 0; }
    void updateHeight(Node* node);

    Node* rotateRight(Node* y);
    Node* rotateLeft(Node* x);
    Node* balance(Node* node);

    Node* insert(Node* node, K key, V value);

    Node* findMin(Node* node) { return node->left ? findMin(node->left) : node; }
    Node* removeMin(Node* node);
    Node* remove(Node* node, K key);

    void destroy(Node* node); // Destructor Helper

private:
    Node* root;
};