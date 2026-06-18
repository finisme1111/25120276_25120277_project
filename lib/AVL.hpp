#ifndef AVL_HPP
#define AVL_HPP

#include <cstddef>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace lib {
    template <typename T, typename Compare = std::less<T>>\
    // Cay AVL
    class AVL {
    private:
        struct Node {
            T data;
            Node* left;
            Node* right;
            int height;

            explicit Node(const T& value): data(value), left(nullptr), right(nullptr), height(1) {}
        };

        Node* root;
        std::size_t sze;
        Compare cmp;

        // ham lay max cua 2 so nguyen
        static int maxHeight(int a, int b) {
            return a > b ? a : b;
        }

        // ham lay chieu cao cua node
        static int getHeight(const Node* node) {
            return node ? node->height : 0;
        }

        // ham tinh do can bang cua node
        static int getBalance(const Node* node) {
            if (!node) return 0;

            return getHeight(node->left) - getHeight(node->right);
        }

        // ham cap nhat chieu cao cua node
        static void updateHeight(Node* node) {
            if (!node) return;

            node->height = 1 + maxHeight(getHeight(node->left), getHeight(node->right));
        }

        // ham so sanh nho hon
        bool isLess(const T& a, const T& b) const {
            return cmp(a, b);
        }

        // ham so sanh bang nhau
        bool isEqual(const T& a, const T& b) const {
            return !cmp(a, b) && !cmp(b, a);
        }

        // ham xoay phai
        static Node* rotateRight(Node* rootNode) {
            Node* newRoot = rootNode->left;
            Node* movedSubTree = newRoot->right;

            newRoot->right = rootNode;
            rootNode->left = movedSubTree;

            updateHeight(rootNode);
            updateHeight(newRoot);

            return newRoot;
        }

        // ham xoay trai
        static Node* rotateLeft(Node* rootNode) {
            Node* newRoot = rootNode->right;
            Node* movedSubTree = newRoot->left;

            newRoot->left = rootNode;
            rootNode->right = movedSubTree;

            updateHeight(rootNode);
            updateHeight(newRoot);

            return newRoot;
        }

        // ham can bang lai cay
        static Node* rebalance(Node* node) {
            if (!node) return nullptr;

            updateHeight(node);

            const int balance = getBalance(node);

            // left-left hoac left-right.
            if (balance > 1) {
                if (getBalance(node->left) < 0) node->left = rotateLeft(node->left);

                return rotateRight(node);
            }

            // right-right hoac right-left.
            if (balance < -1) {
                if (getBalance(node->right) > 0) node->right = rotateRight(node->right);

                return rotateLeft(node);
            }

            return node;
        }

        // ham phu tro them node vao cay
        Node* insertHelper(Node* node, const T& value, bool& inserted) {
            if (!node) {
                Node* newNode = new Node(value);

                ++sze;
                inserted = true;

                return newNode;
            }

            if (isLess(value, node->data)) {
                node->left = insertHelper(node->left, value, inserted);
            } else if (isLess(node->data, value)) {
                node->right = insertHelper(node->right, value, inserted);
            } else {
                inserted = false;
                return node;
            }

            return rebalance(node);
        }

        // ham tim node co gia tri nho nhat trong cay con
        static Node* getMinNode(Node* node) {
            if (!node) return nullptr;

            while (node->left) node = node->left;

            return node;
        }

        // ham tim node co gia tri lon nhat trong cay con
        static const Node* getMaxNode(const Node* node) {
            if (!node) return nullptr;

            while (node->right) node = node->right;

            return node;
        }

        // ham phu tro xoa node khoi cay
        Node* removeHelper(Node* node, const T& value, bool& removed) {
            if (!node) {
                removed = false;
                return nullptr;
            }

            if (isLess(value, node->data)) {
                node->left = removeHelper(node->left, value, removed);
            } else if (isLess(node->data, value)) {
                node->right = removeHelper(node->right, value, removed);
            } else {
                removed = true;

                // node co 0 hoac 1 co
                if (!node->left || !node->right) {
                    Node* replacement = node->left ? node->left : node->right;

                    delete node;
                    --sze;

                    return replacement;
                }

                // node co 2 con, thay bang node nho nhat ben phai
                Node* successor = getMinNode(node->right);

                node->data = successor->data;

                bool successorRemoved = false;

                node->right = removeHelper(node->right, successor->data, successorRemoved);
            }

            return rebalance(node);
        }

        // ham tim node co gia tri bang value
        Node* findNode(Node* node, const T& value) const {
            while (node) {
                if (isLess(value, node->data)) {
                    node = node->left;
                } else if (isLess(node->data, value)) {
                    node = node->right;
                } else {
                    return node;
                }
            }

            return nullptr;
        }

        // ham tim node co gia tri bang value, khong cho phep thay doi du lieu
        const Node* findNode(const Node* node, const T& value) const {
            while (node) {
                if (isLess(value, node->data)) {
                    node = node->left;
                } else if (isLess(node->data, value)) {
                    node = node->right;
                } else {
                    return node;
                }
            }

            return nullptr;
        }

        // ham xoa toan bo cay
        static void destroyTree(Node* node) {
            if (!node) return;
    
            destroyTree(node->left);
            destroyTree(node->right);

            delete node;
        }

        // ham sao chep cay
        static Node* copyTree(const Node* node) {
            if (!node) return nullptr;

            Node* newNode = new Node(node->data);
            newNode->height = node->height;

            try {
                newNode->left = copyTree(node->left);
                newNode->right = copyTree(node->right);
            } catch (...) {
                destroyTree(newNode);
                throw;
            }

            return newNode;
        }

        // ham in cay theo thu tu node-left-right
        static void preorderPrint(const Node* node) {
            if (!node) return;

            std::cout << node->data << ' ';
            preorderPrint(node->left);
            preorderPrint(node->right);
        }

        // ham in cay theo thu tu left-node-right
        static void inorderPrint(const Node* node) {
            if (!node) return;

            inorderPrint(node->left);
            std::cout << node->data << ' ';
            inorderPrint(node->right);
        }

        // ham in cay theo thu tu left-right-node
        static void postorderPrint(const Node* node) {
            if (!node) return;

            postorderPrint(node->left);
            postorderPrint(node->right);
            std::cout << node->data << ' ';
        }

        // ham duyet cay theo thu tu left-node-right voi callback
        template <typename Function>
        static void inorderCallback(Node* node, Function& function) {
            if (!node) return;

            inorderCallback(node->left, function);
            function(node->data);
            inorderCallback(node->right, function);
        }

        // ham duyet cay theo thu tu left-node-right voi callback, khong cho phep thay doi du lieu
        template <typename Function>
        static void inorderCallback(const Node* node, Function& function) {
            if (!node) return;

            inorderCallback(node->left, function);
            function(node->data);
            inorderCallback(node->right, function);
        }

        // ham ho tro swap du lieu giua 2 cay
        void swapTree(AVL& other) {
            Node* temporaryRoot = root;
            root = other.root;
            other.root = temporaryRoot;

            std::size_t temporarySize = sze;
            sze = other.sze;
            other.sze = temporarySize;

            Compare temporaryCompare = std::move(cmp);
            cmp = std::move(other.cmp);
            other.cmp = std::move(temporaryCompare);
        }

    public:
        // khoi tao cay 
        AVL(): root(nullptr), sze(0), cmp(Compare{}) {}

        // khoi tao cay voi ham so sanh tuy chinh
        explicit AVL(const Compare& compare): root(nullptr), sze(0), cmp(compare) {}

        // ham xoa cay
        ~AVL() {
            clear();
        }

        // ham sao chep cay
        AVL(const AVL& other): root(nullptr), sze(0), cmp(other.cmp) {
            root = copyTree(other.root);
            sze = other.sze;
        }

        // ham gan chuyen doi cay
        AVL(AVL&& other): root(other.root), sze(other.sze), cmp(std::move(other.cmp)) {
            other.root = nullptr;
            other.sze = 0;
        }

        // ham sao chep cay
        AVL& operator=(const AVL& other) {
            if (this == &other) return *this;

            AVL temporary(other);
            swapTree(temporary);

            return *this;
        }

        // ham gan chuyen doi cay
        AVL& operator=(AVL&& other) {
            if (this == &other) return *this;

            AVL temporary(std::move(other));
            swapTree(temporary);

            return *this;
        }

        // ham xoa toan bo cay
        void clear() {
            destroyTree(root);

            root = nullptr;
            sze = 0;
        }

        // ham chen node co gia tri value, tra ve true neu chen thanh cong, false neu da ton tai
        bool insert(const T& value) {
            bool inserted = false;

            root = insertHelper(root, value, inserted);

            return inserted;
        }

        // ham xoa node co gia tri bang value, tra ve true neu xoa thanh cong, false neu khong tim thay node de xoa
        bool remove(const T& value) {
            bool removed = false;

            root = removeHelper(root, value, removed);

            return removed;
        }

        // ham kiem tra xem cay co chua gia tri value hay khong
        bool search(const T& value) const {
            return findNode(root, value) != nullptr;
        }

        // ham tim node co gia tri bang value
        T* find(const T& value) {
            Node* node = findNode(root, value);

            return node ? &node->data : nullptr;
        }

        // ham tim node co gia tri bang value, khong cho phep thay doi du lieu
        const T* find(const T& value) const {
            const Node* node = findNode(root, value);

            return node ? &node->data : nullptr;
        }

        // ham tra ve gia tri lon nhat trong cay
        T getMax() const {
            if (!root) {
                throw std::underflow_error("Tree is empty.");
            }

            return getMaxNode(root)->data;
        }

        // ham tra ve so luong node trong cay
        std::size_t size() const {
            return sze;
        }

        // ham kiem tra cay co rong hay khong
        bool empty() const {
            return root == nullptr;
        }

        // ham in cay theo thu tu node-left-right
        void preorder() const {
            preorderPrint(root);
            std::cout << '\n';
        }

        // ham in cay theo thu tu left-node-right
        void inorder() const {
            inorderPrint(root);
            std::cout << '\n';
        }

        // ham in cay theo thu tu left-right-node
        void postorder() const {
            postorderPrint(root);
            std::cout << '\n';
        }

        // ham duyet cay theo thu tu left-node-right voi callback
        template <typename Function>
        void inorder(Function function) {
            inorderCallback(root, function);
        }

        // ham duyet cay theo thu tu left-node-right voi callback, khong cho phep thay doi du lieu
        template <typename Function>
        void inorder(Function function) const {
            inorderCallback(root, function);
        }
    };

}

#endif