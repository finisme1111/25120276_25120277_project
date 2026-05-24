#pragma once
#ifndef PRIORITYQUEUE_HPP
#define PRIORITYQUEUE_HPP
#include "AVL.hpp"
#include <stdexcept>
#include <cstddef>
#include <functional>

namespace lib {
    template<typename T>
    class PriorityQueue {
    private: 
        AVL<T> avlTree;
    public:
        PriorityQueue() : avlTree() {}

        ~PriorityQueue() {}

        PriorityQueue(const PriorityQueue& other) : avlTree(other.avlTree) {}

        PriorityQueue& operator=(const PriorityQueue& other) {
            if (this != & other) {
                avlTree = other.avlTree;
            }
            return *this;
        }

        void push(const T& value) {
            avlTree.insert(value);
        }

        T top() const {
            if (empty()) {
                throw std::underflow_error("Priority Queue is empty.");
            }
            return avlTree.getMax();
        }

        void pop() {
            if (empty()) {
                throw std::underflow_error("Priority Queue is empty.");
            }
            T maxVal = top();
            avlTree.remove(maxVal);
        }

        bool empty() const {
            return avlTree.empty();
        }

        size_t size() const {
            return avlTree.size();
        }

        void clear() {
            avlTree.clear();
        }
    };
}
#endif