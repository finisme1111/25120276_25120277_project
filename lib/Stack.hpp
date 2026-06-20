#ifndef STACK_HPP
#define STACK_HPP

#include "LinkedList.hpp"

#include <cstddef>
#include <stdexcept>

namespace lib {

template <typename T>
// ngan xep
class Stack {
private:
    LinkedList<T> list;

public:
    // khoi tao ngan xep
    Stack() = default;

    // xoa ngan xep
    ~Stack() = default;

    // sao chep ngan xep
    Stack(const Stack&) = default;

    // di chuyen ngan xep
    Stack(Stack&&) noexcept = default;

    // gan ngan xep
    Stack& operator=(const Stack&) = default;

    // gan ngan xep bang cach di chuyen
    Stack& operator=(Stack&&) noexcept = default;

    // ham them gia tri vao ngan xep
    void push(const T& value) {
        list.insertFront(value);
    }

    // ham xoa gia tri khoi ngan xep
    void pop() {
        if (empty()) throw std::underflow_error("Stack underflow: Stack is empty.");

        list.removeFront();
    }

    // ham lay gia tri o dinh ngan xep
    T& top() {
        if (empty()) throw std::underflow_error("Stack underflow: Stack is empty.");

        return list.front();
    }

    // ham lay gia tri o dinh ngan xep, khong thay doi du lieu
    const T& top() const {
        if (empty()) throw std::underflow_error("Stack underflow: Stack is empty.");

        return list.front();
    }

    // ham kiem tra xem ngan xep co rong hay khong
    bool empty() const {
        return list.empty();
    }

    // ham tra ve kich thuoc cua ngan xep
    std::size_t size() const {
        return list.size();
    }

    // ham xoa toan bo gia tri trong ngan xep
    void clear() noexcept {
        list.clear();
    }
};

}

#endif