#pragma once 
#ifndef STACK_HPP
#define STACK_HPP
#include "LinkedList.hpp"
#include <stdexcept>
#include <cstddef>
#include <functional>

namespace lib {
    template<typename T>
    class Stack {
    private:
        LinkedList<T> list;
    public:
        Stack() = default;

        ~Stack() = default;

        Stack(const Stack& other) : list(other.list) {}

        Stack& operator=(const Stack& other) {
            if (this != &other) {
                list = other.list;
            }
            return *this;
        }

        void push(const T& value) {
            list.insertFront(value);
        }

        void pop() {
            if (empty()) {
                throw std::underflow_error("Stack underflow: Stack is empty.");
            }
            list.removeFront();
        }

        T top() {
            if (empty()) {
                throw std::underflow_error("Stack underflow: Stack is empty.");
            }
            return list.front();
        }

        T top() const {
            if (empty()) {
                throw std::underflow_error("Stack underflow: Stack is empty.");
            }
            return list.front();
        }

        bool empty() const {
            return list.empty();
        }

        size_t size() const {
            return list.size();
        }
    };
}
#endif