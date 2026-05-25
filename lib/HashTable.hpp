#pragma once
#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP
#include <stdexcept>
#include <cstddef>
#include <functional>

namespace lib {
    template<typename KeyType, typename ValueType>
    class HashTable {
    public:
        struct Node {
            KeyType key;
            ValueType value;
            Node* next;
            Node(const KeyType& k, const ValueType& v) : key(k), value(v), next(nullptr) {}
        };
        
    private:
        Node** table;
        size_t capacity;
        size_t current_size;

        size_t hashFunction(const KeyType& key) const {
            return std::hash<KeyType>{}(key) % capacity;
        }

        void rehash() {
            size_t oldCapacity = capacity;
            capacity *= 2;

            Node** oldTable = table;
            table = new Node*[capacity]();

            for (size_t i = 0; i < oldCapacity; i++) {
                Node* head = oldTable[i];
                while (head != nullptr) {
                    Node* nextNode = head->next;

                    size_t newIndex = hashFunction(head->key);
                    head->next = table[newIndex];
                    table[newIndex] = head;
                    head = nextNode;
                }
            }
            delete[] oldTable;
        }

    public:
        HashTable(size_t cap = 101) : capacity(cap), current_size(0) {
            table = new Node*[capacity]();
        }

        ~HashTable() {
            clear();
            delete[] table;
        }

        HashTable(const HashTable&) = delete;
        HashTable& operator=(const HashTable&) = delete;

        void insert(const KeyType& key, const ValueType& value) {
            size_t index = hashFunction(key);
            Node* head = table[index];

            while (head != nullptr) {
                if (head->key == key) {
                    head->value = value;
                    return;
                }
                head = head->next;
            }

            if ((current_size + 1) * 4 > capacity * 3) {
                rehash();
                index = hashFunction(key);
            }

            Node* newNode = new Node(key, value);
            newNode->next = table[index];
            table[index] = newNode;
            current_size++;
        }

        bool remove(const KeyType& key) {
            size_t index= hashFunction(key);
            Node* head = table[index];
            Node* prev = nullptr;

            while (head != nullptr) {
                if (head->key == key) {
                    if (prev == nullptr) {
                        table[index] = head->next;
                    }
                    else {
                        prev->next = head->next;
                    }
                    delete head;
                    current_size--;
                    return true;
                }
                prev = head;
                head = head->next;
            }
            return false;
        }

        bool contains(const KeyType& key) const {
            size_t index = hashFunction(key);
            Node* head = table[index];

            while (head != nullptr) {
                if (head->key == key) {
                    return true;
                }
                head = head->next;
            }
            return false;
        }

        Node* findNode(const KeyType& key) {
            size_t index = hashFunction(key);
            Node* head = table[index];

            while (head != nullptr) {
                if (head->key == key) {
                    return head;
                }
                head = head->next;
            }
            return nullptr;
        }

        const Node* findNode(const KeyType& key) const {
            size_t index = hashFunction(key);
            Node* head = table[index];

            while (head != nullptr) {
                if (head->key == key) {
                    return head;
                }
                head = head->next;
            }
            return nullptr;
        }

        /** Tra ve con tro den gia tri (nullptr neu khong tim thay) */
        ValueType* find(const KeyType& key) {
            Node* node = findNode(key);
            return node ? &node->value : nullptr;
        }

        const ValueType* find(const KeyType& key) const {
            size_t index = hashFunction(key);
            Node* head = table[index];
            while (head != nullptr) {
                if (head->key == key) return &head->value;
                head = head->next;
            }
            return nullptr;
        }

        void clear() {
            for (size_t i = 0; i < capacity; i++) {
                Node* head = table[i];
                while (head != nullptr) {
                    Node* temp = head;
                    head = head->next;
                    delete temp;
                }
                table[i] = nullptr;
            }
            current_size = 0;
        }

        size_t size() const {
            return current_size;
        }

        bool empty() const {
            return current_size == 0;
        }

        /** Duyet toan bo bang bam voi callback */
        void forEach(std::function<void(const KeyType&, const ValueType&)> fn) const {
            for (size_t i = 0; i < capacity; i++) {
                Node* head = table[i];
                while (head != nullptr) {
                    fn(head->key, head->value);
                    head = head->next;
                }
            }
        }
    };
}

#endif