
#pragma once

template <typename K, typename V>
class HashTable {
public:
    struct Node {
        K key;
        V value;
        Node* next;
        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

public:
    HashTable(unsigned int size = 8);
    ~HashTable();

    V operator[](const K& key) const { return get(key); }

    void insert(const K& key, const V& value);
    bool remove(const K& key);
    bool contains(const K& key) const;
    V get(const K& key) const;

    unsigned int size() const { return current_size; }

protected:
    int hash(const K& key) const;
    void resize();

protected:
    Node** table;
    unsigned int num_buckets;
    unsigned int current_size;

    constexpr float resize_threshold = 0.75;
};