
#pragma once

#include <vector>
#include <cstring>

template <typename K, typename V>
class HashTable {
public:
    struct Node {
        K key;
        V value;
        Node* next;
        Node(const K& k, V v) : key(k), value(v), next(nullptr) {}
    };

public:
    HashTable(unsigned int size = 1024);
    ~HashTable();

    V get(const K& key) const;
    void put(const K& key, V value);
    bool remove(const K& key);
    bool contains(const K& key) const;
    std::vector<K> keys() const;

    unsigned int size() const { return current_size; }

private:
    unsigned int hash(const K& key) const;
    void resize();

private:
    Node** table;
    unsigned int num_buckets;
    unsigned int current_size;
    const float load_factor_threshold = 0.75;
};