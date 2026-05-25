#pragma once
#ifndef QUEUE_HPP
#define QUEUE_HPP
#include "LinkedList.hpp"
#include <stdexcept>
#include <cstddef>
#include <functional>

namespace lib {
    template<typename T>
    class Queue {
    private:
        LinkedList<T> list;
    public:
        Queue() = default;

        ~Queue() = default;

        Queue(const Queue& other) : list(other.list) {}

        Queue& operator=(const Queue& other) {
            if (this != &other) {
                list = other.list;
            }
            return *this;
        }

        void enqueue(const T& value) {
            list.insertBack(value);
        }

        void dequeue() {
            if (empty()) {
                throw std::underflow_error("Queue underflow: Queue is empty.");
            }
            list.removeFront();
        }

        T front() {
            if (empty()) {
                throw std::underflow_error("Queue underflow: Queue is empty.");
            }
            return list.front();
        }

        T front() const {
            if (empty()) {
                throw std::underflow_error("Queue underflow: Queue is empty.");
            }
            return list.front();
        }

        bool empty() const {
            return list.empty();
        }

        size_t size() const {
            return list.size();
        }

        void clear() {
            list.clear();
        }
    };
}
#endif
