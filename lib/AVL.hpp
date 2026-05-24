#pragma once
#ifndef AVL_HPP
#define AVL_HPP
#include <stdexcept>
#include <cstddef>
#include <functional>
#include <iostream>
#include <algorithm>

namespace lib {
    template<typename T>
    class AVL {
    private:
        struct Node {
            T data;
            Node* left;
            Node* right;
            int height;
            Node(const T& value) : data(value), left(nullptr), right(nullptr), height(1) {}
        };
        
        Node* root;
        size_t sze;

        int getHeight(Node* node) const {
            return node ? node->height : 0;
        }

        int getBalance(Node* node) const {
            return node ? getHeight(node->left) - getHeight(node->right) : 0;
        }

        void updateHeight(Node* node) {
            if (node) {
                node->height = 1 + std::max(getHeight(node->left), getHeight(node->right));
            }
        }

        Node* rotateRight(Node* y) {
            Node* x = y->left;
            Node* T2 = x->right;

            x->right =  y;
            y->left = T2;

            updateHeight(y);
            updateHeight(x);

            return x;
        }

        Node* rotateLeft(Node* x) {
            Node* y = x->right;
            Node* T2 = y->left;

            y->left = x;
            x->right = T2;

            updateHeight(x);
            updateHeight(y);

            return y;
        }

        Node* insertHelper(Node* node, const T& value, bool& inserted) {
            if (!node) {
                inserted =true;
                sze++;
                return new Node(value);
            }

            if (value < node->data) {
                node->left = insertHelper(node->left, value, inserted);
            }
            else if (value > node->data) {
                node->right = insertHelper(node->right, value, inserted);
            }
            else {
                inserted = false;
                return node;
            }

            updateHeight(node);

            int balance = (getBalance(node));

            if (balance > 1 && getBalance(node->left) >= 0) {
                return rotateRight(node);
            }
            if (balance > 1 && getBalance(node->left) < 0) {
                node->left = rotateLeft(node->left);
                return rotateRight(node);
            }
            if (balance < -1 && getBalance(node->right) <= 0) {
                return rotateLeft(node);
            }
            if (balance < -1 && getBalance(node->right) > 0) {
                node->right = rotateRight(node->right);
                return rotateLeft(node);
            }

            return node;
        }

        Node* removeHelper(Node* node, const T& value, bool& removed) {
            if (!node) {
                removed = false;
                return nullptr;
            }

            if (value < node->data) {
                node->left = removeHelper(node->left, value, removed);
            }
            else if (value > node->data) {
                node->right = removeHelper(node->right, value, removed);
            }
            else {
                removed = true;
                if (!node->left || !node->right) {
                    Node* temp = node->left ? node->left : node->right;
                    if (!temp) {
                        temp = node;
                        node = nullptr;
                    }
                    else {
                        *node = *temp;
                    }
                    delete temp;
                    sze--;
                }
                else {
                    Node* minNode = node->right;
                    while(minNode->left) {
                        minNode = minNode->left;
                    }
                    node->data = minNode->data;
                    node->right = removeHelper(node->right, minNode->data, removed);
                }
            }

            if (!node) return nullptr;

            updateHeight(node);

            int balance = getBalance(node);

            if (balance > 1 && getBalance(node->left) >= 0) {
                return rotateRight(node);
            }
            if (balance > 1 && getBalance(node->left) < 0) {
                node->left = rotateLeft(node->left);
                return rotateRight(node);
            }
            if (balance < -1 && getBalance(node->right) <= 0) {
                return rotateLeft(node);
            }
            if (balance < -1 && getBalance(node->right) > 0) {
                node->right = rotateRight(node->right);
                return rotateLeft(node);
            }

            return node;
        }

        bool searchHelper(Node* node, const T& value) const {
            if (!node) return false;
            if (value < node->data) return searchHelper(node->left, value);
            if (value > node->data) return searchHelper(node->right, value);
            return true;
        }

        void clear(Node* node) {
            if (node) {
                clear(node->left);
                clear(node->right);
                delete node;
            }
        }

        Node* copyTree(Node* node) {
            if (!node) return nullptr;
            Node* newNode = new Node(node->data);
            newNode->height = node->height;
            newNode->left = copyTree(node->left);
            newNode->right = copyTree(node->right);
            return newNode;
        }

        void preorderHelper(Node* node) const {
            if (node) {
                std::cout << node->data << " ";
                preorderHelper(node->left);
                preorderHelper(node->right);
            }
        }

        void inorderHelper(Node* node) const {
            if (node) {
                inorderHelper(node->left);
                std::cout << node->data << " ";
                inorderHelper(node->right);
            }
        }

        void postorderHelper(Node* node) const {
            if (node) {
                postorderHelper(node->left);
                postorderHelper(node->right);
                std::cout << node->data << " ";
            }
        }

        T getMaxHelper(Node* node) const {
            while (node && node->right) {
                node = node->right;
            }
            return node->data;
        }

    public:
        AVL() : root(nullptr), sze(0) {}

        ~AVL() {
            clear();
        }

        void clear() {
            clear(root);
            root = nullptr;
            sze = 0;
        }

        AVL(const AVL& other) : root(nullptr), sze(0) {
            root = copyTree(other.root);
            sze = other.sze;
        }

        AVL& operator=(const AVL& other) {
            if (this != &other) {
                clear();
                root = copyTree(other.root);
                sze = other.sze;
            }
            return *this;
        }
        
        bool insert(const T& value) {
            bool inserted = false;
            root = insertHelper(root, value, inserted);
            return inserted;
        }

        bool remove(const T& value) {
            bool removed = false;
            root = removeHelper(root, value, removed);
            return removed;
        }

        bool search(const T& value) const {
            return searchHelper(root, value);
        }

        void preorder() const {
            preorderHelper(root);
            std::cout << '\n';
        }

        void inorder() const {
            inorderHelper(root);
            std::cout << '\n';
        }

        void postorder() const {
            postorderHelper(root);
            std::cout << '\n';
        }

        T getMax() const {
            if (!root) throw std::underflow_error("Tree is empty.");
            return getMaxHelper(root);
        }

        size_t size() const {
            return sze;
        }

        bool empty() const {
            return root == nullptr;
        }
    };
}
#endif