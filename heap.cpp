
#include "heap.h"

#include <stdexcept>

template <typename T>
void Heap<T>::insert(const T& value) {
    data.push_back(value);
    heapifyUp(data.size() - 1);
}

template <typename T>
T Heap<T>::getMin() const {
    if (empty()) {
        throw std::out_of_range("Heap is empty");
    }
    return data[0];
}

template <typename T>
T Heap<T>::extractMin() {
    if (empty()) {
        throw std::out_of_range("Heap is empty");
    }
    T minValue = data[0];
    data[0] = data.back();
    data.pop_back();
    if (!data.empty()) {
        heapifyDown(0);
    }
    return minValue;
}

template <typename T>
void Heap<T>::heapifyUp(int index) {
    while (index != 0 && data[parent(index)] > data[index]) {
        std::swap(data[index], data[parent(index)]);
        index = parent(index);
    }
}

template <typename T>
void Heap<T>::heapifyDown(int index) {
    int size = data.size();
    int smallest = index;

    int left = leftChild(index);
    int right = rightChild(index);

    if (left < size && data[left] < data[smallest]) {
        smallest = left;
    }
    if (right < size && data[right] < data[smallest]) {
        smallest = right;
    }

    if (smallest != index) {
        std::swap(data[index], data[smallest]);
        heapifyDown(smallest);
    }
}
