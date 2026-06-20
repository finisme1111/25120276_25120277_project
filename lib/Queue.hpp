#ifndef QUEUE_HPP
#define QUEUE_HPP

#include "LinkedList.hpp"

#include <cstddef>
#include <stdexcept>

namespace lib {

template <typename T>
// hang doi
class Queue {
private:
    LinkedList<T> list;

public:
    // khoi tao hang doi
    Queue() = default;

    // xoa hang doi
    ~Queue() = default;

    // sao chep hang doi
    Queue(const Queue&) = default;

    // di chuyen hang doi
    Queue(Queue&&) noexcept = default;

    // gan hang doi
    Queue& operator=(const Queue&) = default;

    // gan hang doi bang cach di chuyen
    Queue& operator=(Queue&&) noexcept = default;

    // ham them gia tri vao hang doi
    void enqueue(const T& value) {
        list.insertBack(value);
    }

    // ham xoa gia tri khoi hang doi
    void dequeue() {
        if (empty()) throw std::underflow_error("Queue underflow: Queue is empty.");

        list.removeFront();
    }

    // ham lay gia tri o dau hang doi
    T& front() {
        if (empty()) throw std::underflow_error("Queue underflow: Queue is empty.");

        return list.front();
    }

    // ham lay gia tri o dau hang doi, khong thay doi du lieu
    const T& front() const {
        if (empty()) throw std::underflow_error("Queue underflow: Queue is empty.");

        return list.front();
    }

    // ham kiem tra hang doi co rong hay khong
    bool empty() const {
        return list.empty();
    }

    // ham tra ve kich thuoc cua hang doi
    std::size_t size() const {
        return list.size();
    }

    // ham xoa toan bo phan tu trong hang doi
    void clear() noexcept {
        list.clear();
    }
};

}

#endif