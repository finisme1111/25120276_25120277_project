#pragma once
#include <stdexcept>
#include <cstddef>

namespace lib {
    template <typename T>
    class Queue {
    private:
        lib::LinkedList<T> list;
    public:
        Queue() = default;

        ~Queue() = default;

        Queue(const T& other) = default;

        Queue& operator=(const T& other) = default;

        void push(const T& value) {
            list.push_back();
        }

        void pop() {
            if (list.empty()) {
                throw std::out_of_range("Queue is empty.");
            }
            list.pop_back();
        }

        const T& top() {
            if (list.empty()) {
                throw std::out_of_range("Queue is empty.");
            }
            return list.front();
        }

        T& top() {
            if (list.empty()) {
                throw std::out_of_range("Queue is empty.");
            }
            return list.front();
        }

        bool empty() {
            return list.size() == 0;
        }

        size_t size() {
            return list.size();
        }

        void clear() {
            list.clear();
        }
    };
}