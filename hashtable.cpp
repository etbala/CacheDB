
#include "hashtable.h"

template <typename K, typename V>
HashTable<K,V>::HashTable(unsigned int size) {
    this->num_buckets = size;
    this->current_size = 0;
    
    table = new Node*[num_buckets];
    std::memset(table, 0, num_buckets * sizeof(Node*));
}

template <typename K, typename V>
HashTable<K,V>::~HashTable() {
    for (unsigned int i = 0; i < num_buckets; ++i) {
        Node* entry = table[i];
        while (entry) {
            Node* prev = entry;
            entry = entry->next;
            delete prev->value; // If V is a pointer, delete the object it points to
            delete prev;
        }
    }
    delete[] table;
}

template <typename K, typename V>
void HashTable<K,V>::put(const K& key, V value) {
    if ((float)current_size / num_buckets > load_factor_threshold) {
        resize();
    }

    unsigned int index = hash(key);
    Node* entry = table[index];
    Node* prev = nullptr;

    while (entry && entry->key != key) {
        prev = entry;
        entry = entry->next;
    }

    if (!entry) {
        entry = new Node(key, value);
        if (!prev) {
            table[index] = entry;
        } else {
            prev->next = entry;
        }
        current_size++;
    } else {
        delete entry->value; // If V is a pointer, delete the old object
        entry->value = value;
    }
}

template <typename K, typename V>
V HashTable<K,V>::get(const K& key) const {
    unsigned int index = hash(key);
    Node* entry = table[index];

    while (entry) {
        if (entry->key == key) {
            return entry->value;
        }
        entry = entry->next;
    }
    return nullptr; // Key not found
}

template <typename K, typename V>
bool HashTable<K,V>::remove(const K& key) {
    unsigned int index = hash(key);
    Node* entry = table[index];
    Node* prev = nullptr;

    while (entry && entry->key != key) {
        prev = entry;
        entry = entry->next;
    }

    if (!entry) {
        return false;
    }

    if (!prev) {
        table[index] = entry->next;
    } else {
        prev->next = entry->next;
    }

    delete entry->value; // Delete the value if V is a pointer
    delete entry;
    current_size--;
    return true;
}

template <typename K, typename V>
bool HashTable<K,V>::contains(const K& key) const {
    return get(key) != nullptr;
}

template <typename K, typename V>
std::vector<K> HashTable<K,V>::keys() const {
    std::vector<K> result;
    for (unsigned int i = 0; i < num_buckets; ++i) {
        Node* entry = table[i];
        while (entry) {
            result.push_back(entry->key);
            entry = entry->next;
        }
    }
    return result;
}

template <typename K, typename V>
unsigned int HashTable<K,V>::hash(const K& key) const {
    size_t key_hash = std::hash<K>()(key);
    return key_hash % num_buckets;
}

template <typename K, typename V>
void HashTable<K,V>::resize() {
    unsigned int new_num_buckets = num_buckets * 2;
    Node** new_table = new Node*[new_num_buckets];
    std::memset(new_table, 0, new_num_buckets * sizeof(Node*));

    for (unsigned int i = 0; i < num_buckets; ++i) {
        Node* entry = table[i];
        while (entry) {
            Node* next = entry->next;
            unsigned int new_index = hash(entry->key) % new_num_buckets;
            entry->next = new_table[new_index];
            new_table[new_index] = entry;
            entry = next;
        }
    }

    delete[] table;
    table = new_table;
    num_buckets = new_num_buckets;
}