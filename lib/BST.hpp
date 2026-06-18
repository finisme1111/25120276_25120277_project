#pragma once
#ifndef BST_HPP
#define BST_HPP

#include <cstddef>
#include <iostream>

namespace lib {

template <typename T>
// cay BST
class BST {
private:
    struct Node {
        T data;
        Node* left;
        Node* right;

        explicit Node(const T& value): data(value), left(nullptr), right(nullptr) {}
    };

    Node* root;
    std::size_t sze;

    // ham xoa toan bo cay
    static void destroy(Node* node) {
        if (!node) return;

        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    // ham sao chep cay
    static Node* copyTree(const Node* node) {
        if (!node) return nullptr;

        Node* newNode = new Node(node->data);

        try {
            newNode->left = copyTree(node->left);
            newNode->right = copyTree(node->right);
        } catch (...) {
            destroy(newNode);
            throw;
        }

        return newNode;
    }

    // ham phu tro them node vao cay
    bool insertHelper(Node*& node, const T& value) {
        if (!node) {
            node = new Node(value);
            ++sze;
            return true;
        }

        if (value < node->data) return insertHelper(node->left, value);

        if (node->data < value) return insertHelper(node->right, value);

        return false;
    }

    // ham phu tro tim kiem node trong cay
    bool searchHelper(const Node* node, const T& value) const {
        while (node) {
            if (value < node->data) node = node->left;
            else if (node->data < value) node = node->right;
            else return true;
        }

        return false;
    }

    // ham phu tro xoa node khoi cay
    bool removeHelper(Node*& node, const T& value) {
        if (!node) return false;

        if (value < node->data) return removeHelper(node->left, value);

        if (node->data < value) return removeHelper(node->right, value);

        // node co 0 con
        if (!node->left) {
            Node* oldNode = node;
            node = node->right;
            delete oldNode;
            --sze;
            return true;
        }

        // node co 1 con
        if (!node->right) {
            Node* oldNode = node;
            node = node->left;
            delete oldNode;
            --sze;
            return true;
        }

        // node co 2 con, tra ve node nho nhat trong cay con phai de thay the
        Node* successor = node->right;

        while (successor->left)
            successor = successor->left;

        node->data = successor->data;

        return removeHelper(node->right, successor->data);
    }

    // ham phu tro duyet cay theo thu tu node-left-right
    static void preorderHelper(const Node* node) {
        if (!node) return;

        std::cout << node->data << ' ';
        preorderHelper(node->left);
        preorderHelper(node->right);
    }

    // ham phu tro duyet cay theo thu tu left-node-right
    static void inorderHelper(const Node* node) {
        if (!node) return;

        inorderHelper(node->left);
        std::cout << node->data << ' ';
        inorderHelper(node->right);
    }

    // ham phu tro duyet cay theo thu tu left-right-node
    static void postorderHelper(const Node* node) {
        if (!node) return;

        postorderHelper(node->left);
        postorderHelper(node->right);
        std::cout << node->data << ' ';
    }

public:
    // ham khoi tao cay BST
    BST() : root(nullptr), sze(0) {}

    // ham xoa cay BST
    ~BST() {
        clear();
    }

    // ham sao chep cay BST
    BST(const BST& other): root(copyTree(other.root)), sze(other.sze) {}

    // ham gan cay BST
    BST& operator=(const BST& other) {
        if (this != &other) {
            Node* newRoot = copyTree(other.root);

            destroy(root);
            root = newRoot;
            sze = other.sze;
        }

        return *this;
    }

    // ham xoa toan bo cay BST
    void clear() {
        destroy(root);
        root = nullptr;
        sze = 0;
    }

    // ham them node vao cay BST
    bool insert(const T& value) {
        return insertHelper(root, value);
    }

    // ham xoa node khoi cay BST
    bool remove(const T& value) {
        return removeHelper(root, value);
    }

    // ham tim kiem node trong cay BST
    bool search(const T& value) const {
        return searchHelper(root, value);
    }

    // ham duyet cay theo thu tu node-left-right    
    void preorder() const {
        preorderHelper(root);
        std::cout << '\n';
    }

    // ham duyet cay theo thu tu left-node-right
    void inorder() const {
        inorderHelper(root);
        std::cout << '\n';
    }

    // ham duyet cay theo thu tu left-right-node
    void postorder() const {
        postorderHelper(root);
        std::cout << '\n';
    }

    // ham tra ve so luong node trong cay BST
    std::size_t size() const {
        return sze;
    }

    // ham kiem tra cay BST co rong hay khong
    bool empty() const {
        return root == nullptr;
    }
};

}

#endif