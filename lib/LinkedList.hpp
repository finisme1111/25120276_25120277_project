#pragma once
#ifndef LINKEDLIST_HPP
#define LINKEDLIST_HPP
#include <stdexcept>
#include <cstddef>
#include <functional>
#include <iostream>

namespace lib {
    template<typename T>
    class LinkedList {
    private:
        struct Node {
            T data;
            Node* next;
            Node* prev;
            Node(const T& value) : data(value), next(nullptr), prev(nullptr) {}
        };
        Node *head;
        Node *tail;
        size_t sze;

    public: 
        LinkedList() : head(nullptr), tail(nullptr), sze(0) {}

        ~LinkedList() {
            clear();
        }

        LinkedList(const LinkedList& other) : head(nullptr), tail(nullptr), sze(0) {
            Node* current = other.head;
            while (current) {
                insertBack(current->data);
                current = current->next;
            }
        }

        LinkedList& operator=(const LinkedList& other) {
            if (this != &other) {
                clear();
                Node* current = other.head;
                while (current) {
                    insertBack(current->data);
                    current = current->next;
                }
            }
            return *this;
        }

        void insertFront(const T& value) {
            Node* newNode = new Node(value);
            if (!head) {
                head = tail = newNode;
            }
            else {
                newNode->next = head;
                head->prev = newNode;
                head = newNode;
            }
            sze++;
        }

        void insertBack(const T& value) {
            Node* newNode = new Node(value);
            if (!tail) {
                head = tail = newNode;
            }
            else {
                tail->next = newNode;
                newNode->prev = tail;
                tail = newNode;
            }
            sze++;
        }

        void insertAt(size_t index, const T& value) {
            if (index > sze) {
                throw std::out_of_range("Index out of range.");
            }
            if (index == 0) {
                insertFront(value);
            }
            else if (index == sze) {
                insertBack(value);
            }
            else {
                Node* current = head;
                for (size_t i = 0; i < index; i++) {
                    current = current->next;
                }
                Node* newNode = new Node(value);
                newNode->prev = current->prev;
                newNode->next = current;
                current->prev->next = newNode;
                current->prev = newNode;
                sze++;
            }
        }

        void removeFront() {
            if (empty()) {
                throw std::out_of_range("List is empty.");
            }
            Node* toDelete = head;
            head = head->next;
            if (head) {
                head->prev = nullptr;
            }
            else {
                tail = nullptr;
            }
            delete toDelete;
            sze--;
        }

        void removeBack() {
            if (empty()) {
                throw std::out_of_range("List is empty.");
            }
            Node* toDelete = tail;
            tail = tail->prev;
            if (tail) {
                tail->next = nullptr;
            }
            else {
                head = nullptr;
            }
            delete toDelete;
            sze--;
        }

        void remove(const T& value) {
            Node* current = head;
            while (current) {
                if (current->data == value) {
                    if (current == head) {
                        removeFront();
                    }
                    else if (current == tail) {
                        removeBack();
                    }
                    else {
                        current->prev->next = current->next;
                        current->next->prev = current->prev;
                        delete current;
                        sze--;
                    }
                    return;
                }
                current = current->next;
            }
        }

        void removeAt(size_t index) {
            if (index >= sze) {
                throw std::out_of_range("Index out of range.");
            }
            if (index == 0) {
                removeFront();
            }
            else if (index == sze - 1) {
                removeBack();
            }
            else {
                Node* toDelete = head;
                for (size_t i = 0; i < index; i++) {
                    toDelete = toDelete->next;
                }
                toDelete->prev->next =toDelete->next;
                toDelete->next->prev = toDelete->prev;
                delete toDelete;
                sze--;
            }
        }

        size_t find(const T& value) {
            Node* current = head;
            size_t index = 0;
            while (current) {
                if (current->data == value) {
                    return index;
                }
                current = current->next;
                index++;
            }
            return static_cast<size_t>(-1);
        }

        size_t size() const {
            return sze;
        }

        bool empty() const {
            return sze == 0;
        }

        void clear() { 
            while (!empty()) {
                removeFront();
            }
        }

        T front() {
            if (empty()) {
                throw std::out_of_range("List is empty.");
            }
            return head->data;
        }

        T back() {
            if (empty()) {
                throw std::out_of_range("List is empty.");
            }
            return tail->data;
        }

        T front() const {
            if (empty()) {
                throw std::out_of_range("List is empty.");
            }
            return head->data;
        }

        T back() const {
            if (empty()) {
                throw std::out_of_range("List is empty.");
            }
            return tail->data;
        }

        void printForward() const {
            Node* current = head;
            while (current) {
                std::cout << current->data << " ";
                current = current->next;
            }
            std::cout << '\n';
        }

        void printBackward() const {
            Node* current = tail;
            while (current) {
                std::cout << current->data << " ";
                current = current->prev;
            }
            std::cout << '\n';
        }

        /** Lay phan tu tai vi tri index (0-based) */
        T& get(size_t index) {
            if (index >= sze) throw std::out_of_range("Index out of range.");
            Node* current = head;
            for (size_t i = 0; i < index; i++) current = current->next;
            return current->data;
        }

        const T& get(size_t index) const {
            if (index >= sze) throw std::out_of_range("Index out of range.");
            Node* current = head;
            for (size_t i = 0; i < index; i++) current = current->next;
            return current->data;
        }

        /** Duyet tung phan tu tu dau den cuoi */
        void forEach(std::function<void(const T&)> fn) const {
            Node* current = head;
            while (current) {
                fn(current->data);
                current = current->next;
            }
        }

        /** Duyet tung phan tu tu cuoi ve dau */
        void forEachReverse(std::function<void(const T&)> fn) const {
            Node* current = tail;
            while (current) {
                fn(current->data);
                current = current->prev;
            }
        }
    };
}

#endif