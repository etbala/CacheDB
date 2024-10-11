
#pragma once

template <typename T>
class List {
public:
    class Node {
    public:
        T val;
        Node* next;
        Node* prev;

        Node(T v) : val(v) {}
    };

public:
    List();
    ~List();

    bool empty() const { return head; }
    void remove(Node* node);
    void insert(Node* target, Node* new_node);

    void push_front(T value);
    void push_back(T value);

private:
    Node* head;
    Node* tail;
};
