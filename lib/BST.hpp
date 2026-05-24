#pragma once
#ifndef BST_HPP
#define BST_HPP
#include <cstddef>
#include <stdexcept>
#include <functional>
#include <iostream>

namespace lib {
    template<typename T>
    class BST {
    private:
        struct Node {
            T data;
            Node* left;
            Node* right;
            Node(const T& value) : data(value), left(nullptr), right(nullptr) {}
        };
        Node* root;
        size_t sze;

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
            newNode->left = copyTree(node->left);
            newNode->right = copyTree(node->right);
            return newNode;
        }

        bool insertHelper(Node*& node, const T& value) {
            if (!node) {
                node = new Node(value);
                sze++;
                return true;
            }
            if (value < node->data) return insertHelper(node->left, value);
            if (value > node->data) return insertHelper(node->right, value);
            return false;
        }

        bool searchHelper(Node* node, const T& value) const {
            if (!node) return false;
            if (value < node->data) return searchHelper(node->left, value);
            if (value > node->data) return searchHelper(node->right, value);
            return true;
        }

        bool removeHelper(Node*& node, const T& value) {
            if (!node) return false;

            if (value < node->data) return removeHelper(node->left, value);
            if (value > node->data) return removeHelper(node->right, value);

            if (!node->left) {
                Node* temp = node->right;
                delete node;
                node = temp;
            }
            else if (!node->right) {
                Node* temp = node->left;
                delete node;
                node = temp;
            }
            else {
                Node* minNode = node->right;
                while (minNode->left) {
                    minNode = minNode->left;
                }
                node->data = minNode->data;
                removeHelper(node->right, node->data);
                sze++;
            }
            sze--;
            return true;
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

    public:
        BST() : root(nullptr), sze(0) {}

        ~BST() {
            clear();
        }

        void clear() {
            clear(root);
            root = nullptr;
            sze = 0;
        }

        BST(const BST& other) : root(nullptr), sze(0) {
            root = copyTree(other.root);
            sze = other.sze;
        }

        BST& operator=(const BST& other) {
            if (this != &other) {
                clear();
                root = copyTree(other.root);
                sze = other.sze;
            }
            return *this;
        }

        bool insert(const T& value) {
            return insertHelper(root, value);
        }

        bool remove(const T& value) {
            return removeHelper(root, value);
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

        size_t size() const {
            return sze;
        }

        bool empty() const {
            return root == nullptr;
        }
    };
}
#endif