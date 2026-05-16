#pragma once
#include <cstddef>

namespace lib {
    template <typename T>
    class LinkedList {
    private:
        Struct Node {
            T data;
            Node* next;
            Node(const T& value) : data(value), next(nullptr) {}
        };

        Node* head;
        Node* tail;
        size_t m_size;

    public:
        LinkedList() : head(nullptr), tail(nullptr), m_size(0) {}

        ~LinkedList() {clear();}

        LinkedList(const LinkedList& other) : head(nullptr), tail(nullptr), m_size(0) {
            Node* cur = other.head;
            while (cur) {
                push_back(cur->data);
                cur = cur->next;
            }
        }

        LinkedList& operator=(const LinkedList& other) {
            if (this != &other) {
                clear();
                Node* cur = other.head;
                while (cur) {
                    push_back(cur->data);
                    cur = cur->next;
                }
            }
            return *this;
        }

        void push_back(const T& value) {
            Node * new_node = new Node(value);
            if (!head) {
                head = tail = new_node;
            }
            else {
                tail->next = new_node;
                tail = new_node;
            }
            m_size++;
        }

        void push_front(const T& value) {
            Node* new_node = new Node(value);
            if (!head) {
                head = tail = new_node;
            }
            else {
                new_node->next = head;
                head = new_node;
            }
            m_size++;
        }

        void pop_back() {
            if (!head) {
                throw std::out_of_range("List is empty.");
            }
            if (head == tail) {
                delete head;
                head = tail = nullptr;
            }
            else {
                Node* cur = head;
                while (cur->next != tail) {
                    cur = cur->next;
                }
                delete tail;
                tail = cur;
                tail->next = nullptr;
            }
            m_size--;
        }

        void pop_front() {
            if (!head) {
                throw std::out_of_range("List is empty.");
            }
            Node* tmp = head;
            head = head->next;
            delete tmp;
            if (!head) {
                tail = nullptr;

            }
            m_size--;
        }

        void clear() {
            Node* cur = head;
            while (cur) {
                Node* next = cur->next;
                delete cur;
                cur = next;
            }
            head = tail = nullptr;
            m_size = 0;
        }

        size_t size() {
            return m_size;
        }

        size_t size() const {
            return m_size;
        }

        bool empty() {
            return m_size == 0;
        }

        const T& front() {
            if (!head) {
                throw std::out_of_range("List is empty.");
            }
            return head->data;
        }

        T& front() {
            if (!head) {
                throw std::out_of_range("List is empty.");
            }
            return head->data;
        }

        const T& back() {
            if (!tail) {
                throw std::out_of_range("List is empty.");
            }
            return tail->data;
        }

        T& back() {
            if (!tail) {
                throw std::out_of_range("List is empty.");
            }
            return tail->data;
        }
    };
}