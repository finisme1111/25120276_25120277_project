#pragma once
#ifndef PRIORITYQUEUE_HPP
#define PRIORITYQUEUE_HPP

#include "AVL.hpp"

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>

namespace lib {

template <typename T, typename Compare = std::less<T>>
// hang doi uu tien su dung cay AVL
class PriorityQueue {
private:
    // entry trong hang doi uu tien, chua gia tri va thu tu chen
    struct Entry {
        T value;
        std::size_t order;

        Entry(const T& value, std::size_t order): value(value), order(order) {}
    };

    // so sanh entry theo gia tri, neu bang nhau thi so sanh theo thu tu chen
    struct EntryCompare {
        Compare cmp;

        explicit EntryCompare(const Compare& compare = Compare{}): cmp(compare) {}

        // tra ve true neu first co uu tien hon second
        bool operator()(const Entry& first, const Entry& second) const {
            if (cmp(first.value, second.value)) {
                return true;
            }

            if (cmp(second.value, first.value)) {
                return false;
            }
            return first.order > second.order;
        }
    };

    AVL<Entry, EntryCompare> avlTree;
    std::size_t nextOrder;

public:
    // khoi tao hang doi uu tien rong
    PriorityQueue(): avlTree(EntryCompare{}), nextOrder(0) {}

    // khoi tao hang doi uu tien voi so sanh tuong ung
    explicit PriorityQueue(const Compare& compare): avlTree(EntryCompare{compare}), nextOrder(0) {}

    // xoa hang doi uu tien
    ~PriorityQueue() = default;

    // ham sao chep
    PriorityQueue(const PriorityQueue&) = default;

    // ham chuyen doi
    PriorityQueue(PriorityQueue&&) noexcept = default;

    // ham gan
    PriorityQueue& operator=(const PriorityQueue&) = default;

    // ham gan bang chuyen doi
    PriorityQueue& operator=(PriorityQueue&&) noexcept = default;

    // ham them gia tri vao hang doi uu tien
    void push(const T& value) {
        Entry newEntry(value, nextOrder);

        if (avlTree.insert(newEntry)) {
            ++nextOrder;
        }
    }

    // ham lay gia tri co uu tien cao nhat
    T top() const {
        if (empty()) throw std::underflow_error("Priority Queue is empty.");

        return avlTree.getMax().value;
    }

    // ham xoa gia tri co uu tien cao nhat
    void pop() {
        if (empty()) throw std::underflow_error("Priority Queue is empty.");

        Entry maximum = avlTree.getMax();
        avlTree.remove(maximum);
    }

    // ham kiem tra xem hang doi uu tien co rong hay khong
    bool empty() const {
        return avlTree.empty();
    }

    // ham lay so luong phan tu trong hang doi uu tien
    std::size_t size() const {
        return avlTree.size();
    }

    // ham xoa toan bo phan tu trong hang doi uu tien
    void clear() {
        avlTree.clear();
        nextOrder = 0;
    }
};

}

#endif