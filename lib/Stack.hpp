#pragma once
#include "LinkedList.hpp"
#include <stdexcept>
#include <cstddef>

namespace lib {
    template <typename T>
    class Stack {
    private:
        lib::LinkedList<T> list;
    public:
        Stack() = default;

        ~Stack() = default;

        Stack(const Stack& other) = default;

        Stack& operator=(const Stack& other) = default;

        void push(const T& value) {
            list.push_front(T);
        }

        void pop() {
            if (list.empty()) {
                throw std::out_of_range("Stack is empty.");
            }
            list.pop_back();
        }

        const T& top() {
            if (list.empty()) {
                throw std::out_of_range("Stack is empty.");
            }
            return list.front();
        }

        T& top() {
            if (list.empty()) {
                throw std::out_of_range("Stacks is empty.");
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