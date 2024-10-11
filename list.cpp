
#include "list.h"

template <typename T>
List<T>::List() : head(nullptr), tail(nullptr) {}

template <typename T>
List<T>::~List() {
    Node* current = head;
    while (current != nullptr) {
        Node* temp = current;
        current = current->next;
        delete temp;
    }
}

template <typename T>
void List<T>::remove(Node* node) {
    if (!node) return;

    if (node->prev) {
        node->prev->next = node->next;
    } else {
        // Node is head
        head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        // Node is tail
        tail = node->prev;
    }

    delete node;
}

template <typename T>
void List<T>::insert(Node* target, Node* new_node) {
    if (!new_node) return;

    if (!target) {
        // Insert at the end
        new_node->prev = tail;
        new_node->next = nullptr;

        if (tail) {
            tail->next = new_node;
        } else {
            // List is empty
            head = new_node;
        }
        tail = new_node;
    } else {
        new_node->next = target;
        new_node->prev = target->prev;

        if (target->prev) {
            target->prev->next = new_node;
        } else {
            // Target is head
            head = new_node;
        }
        target->prev = new_node;
    }
}

template <typename T>
void List<T>::push_front(T value) {
    Node* new_node = new Node(value);
    insert(head, new_node);
}

template <typename T>
void List<T>::push_back(T value) {
    Node* new_node = new Node(value);
    insert(nullptr, new_node);
}