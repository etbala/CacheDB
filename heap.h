
#pragma once

#include <vector>

template <typename T>
class Heap {
public:
    Heap() {}

    void insert(const T& value);
    T extractMin();
    T getMin() const;
    bool empty() const { return data.empty(); }

private:
    void heapifyUp(int index);
    void heapifyDown(int index);
    int parent(int index) const { return (index - 1) / 2; }
    int leftChild(int index) const { return (2 * index) + 1; }
    int rightChild(int index) const { return (2 * index) + 2; }

private:
    std::vector<T> data;
};