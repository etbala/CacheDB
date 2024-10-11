
#include "hashtable.h"

#include <stdexcept>

template <typename K, typename V>
HashTable<K,V>::HashTable(unsigned int size)
{
    this->num_buckets = size;
    this->current_size = 0;

    table = new Node*[num_buckets];
    for (int i = 0; i < num_buckets; ++i) {
        table[i] = nullptr;
    }
}

template <typename K, typename V>
HashTable<K,V>::~HashTable()
{
    for (int i = 0; i < num_buckets; ++i) {
        Node* entry = table[i];
        while (entry != nullptr) {
            Node* prev = entry;
            entry = entry->next;
            delete prev;
        }
    }
    delete[] table;
}

template <typename K, typename V>
auto HashTable<K,V>::insert(const K& key, const V& value) -> void
{
    if ((float)current_size / num_buckets > load_factor_threshold) {
        resize();
    }

    int index = hash(key);
    Node* entry = table[index];
    Node* prev = nullptr;

    while (entry != nullptr && entry->key != key) {
        prev = entry;
        entry = entry->next;
    }

    if (entry == nullptr) {
        entry = new Node(key, value);
        if (prev == nullptr) {
            table[index] = entry;
        } else {
            prev->next = entry;
        }
        current_size++;
    } else {
        entry->value = value;
    }
}

template <typename K, typename V>
auto HashTable<K,V>::remove(const K& key) -> bool
{
    int index = hash(key);
    Node* entry = table[index];
    Node* prev = nullptr;

    while (entry != nullptr && entry->key != key) {
        prev = entry;
        entry = entry->next;
    }

    if (entry == nullptr) {
        return false;
    }

    if (prev == nullptr) {
        table[index] = entry->next;
    } else {
        prev->next = entry->next;
    }
    delete entry;
    current_size--;
    return true;
}

template <typename K, typename V>
auto HashTable<K,V>::contains(const K& key) const -> bool
{
    int index = hash(key);
    Node* entry = table[index];

    while (entry != nullptr) {
        if (entry->key == key) {
            return true;
        }
        entry = entry->next;
    }
    return false;
}

template <typename K, typename V>
auto HashTable<K,V>::get(const K& key) const -> V
{
    int index = hash(key);
    Node* entry = table[index];

    while (entry != nullptr) {
        if (entry->key == key) {
            return entry->value;
        }
        entry = entry->next;
    }

    throw std::runtime_error("Key not in hashtable.");
}

template <typename K, typename V>
auto HashTable<K,V>::hash(const K& key) const -> int
{
    unsigned long seed = 5381;
    for (const char& c : key) {
        seed = ((seed << 5) + seed) + c;
    }
    return seed % num_buckets;
}

template <typename K, typename V>
auto HashTable<K,V>::resize() -> void
{
    int new_num_buckets = num_buckets * 2;
    Node** new_table = new Node*[new_num_buckets];
    for (int i = 0; i < new_num_buckets; ++i) {
        new_table[i] = nullptr;
    }

    for (int i = 0; i < num_buckets; ++i) {
        Node* entry = table[i];
        while (entry != nullptr) {
            Node* next = entry->next;
            int new_index = std::hash<K>{}(entry->key) % new_num_buckets;
            entry->next = new_table[new_index];
            new_table[new_index] = entry;
            entry = next;
        }
    }

    delete[] table;
    table = new_table;
    num_buckets = new_num_buckets;
}