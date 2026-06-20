#ifndef LINKEDLIST_HPP
#define LINKEDLIST_HPP

#include <cstddef>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace lib {

template <typename T>
// danh sach lien ket doi
class LinkedList {
private:
    struct Node {
        T data;
        Node* next;
        Node* prev;

        explicit Node(const T& value): data(value), next(nullptr), prev(nullptr) {}
    };

    Node* head;
    Node* tail;
    std::size_t sze;

    // ham truy cap node tri vi tri index
    Node* getNode(std::size_t index) {
        if (index < sze / 2) {
            Node* current = head;

            for (std::size_t i = 0; i < index; ++i) {
                current = current->next;
            }

            return current;
        }

        Node* current = tail;

        for (std::size_t i = sze - 1; i > index; --i) {
            current = current->prev;
        }

        return current;
    }

    // ham truy cap node vi tri index, khong thay doi du lieu
    const Node* getNode(std::size_t index) const {
        if (index < sze / 2) {
            const Node* current = head;

            for (std::size_t i = 0; i < index; ++i) {
                current = current->next;
            }

            return current;
        }

        const Node* current = tail;

        for (std::size_t i = sze - 1; i > index; --i) {
            current = current->prev;
        }

        return current;
    }

    // ham doi noi bo giua 2 danh sach lien ket
    void swapMembers(LinkedList& other) noexcept {
        Node* tempHead = head;
        head = other.head;
        other.head = tempHead;

        Node* tempTail = tail;
        tail = other.tail;
        other.tail = tempTail;

        std::size_t tempSize = sze;
        sze = other.sze;
        other.sze = tempSize;
    }

public:
    // gia tri tra ve khi khong tim thay
    inline static constexpr std::size_t npos =
        static_cast<std::size_t>(-1);

    // ham khoi tao danh sach lien ket
    LinkedList(): head(nullptr), tail(nullptr), sze(0) {}

    // ham xoa danh sach lien ket
    ~LinkedList() {
        clear();
    }

    // ham sao chep danh sach lien ket
    LinkedList(const LinkedList& other): head(nullptr), tail(nullptr), sze(0) {
        try {
            const Node* current = other.head;

            while (current) {
                insertBack(current->data);
                current = current->next;
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    // ham di chuyen danh sach lien ket
    LinkedList(LinkedList&& other) noexcept: head(other.head), tail(other.tail), sze(other.sze) {
        other.head = nullptr;
        other.tail = nullptr;
        other.sze = 0;
    }

    // ham gan danh sach lien ket
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            LinkedList temporary(other);
            swapMembers(temporary);
        }

        return *this;
    }

    // ham gan danh sach lien ket bang cach di chuyen
    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            clear();

            head = other.head;
            tail = other.tail;
            sze = other.sze;

            other.head = nullptr;
            other.tail = nullptr;
            other.sze = 0;
        }

        return *this;
    }

    // ham chen gia tri vao dau danh sach lien ket
    void insertFront(const T& value) {
        Node* newNode = new Node(value);

        newNode->next = head;

        if (head) head->prev = newNode;
        else tail = newNode;

        head = newNode;
        ++sze;
    }

    // ham chen gia tri vao cuoi danh sach lien ket
    void insertBack(const T& value) {
        Node* newNode = new Node(value);

        newNode->prev = tail;

        if (tail) tail->next = newNode;
        else head = newNode;
        
        tail = newNode;
        ++sze;
    }

    // ham chen gia tri vao vi tri index trong danh sach lien ket
    void insertAt(std::size_t index, const T& value) {
        if (index > sze) throw std::out_of_range("Index out of range.");

        if (index == 0) {
            insertFront(value);
            return;
        }

        if (index == sze) {
            insertBack(value);
            return;
        }

        Node* current = getNode(index);
        Node* newNode = new Node(value);

        newNode->prev = current->prev;
        newNode->next = current;

        current->prev->next = newNode;
        current->prev = newNode;

        ++sze;
    }

    // ham xoa phan tu o dau danh sach lien ket
    void removeFront() {
        if (empty()) throw std::out_of_range("List is empty.");

        Node* oldHead = head;
        head = head->next;

        if (head) head->prev = nullptr;
        else tail = nullptr;
        
        delete oldHead;
        --sze;
    }

    // ham xoa phan tu o cuoi danh sach lien ket
    void removeBack() {
        if (empty()) throw std::out_of_range("List is empty.");

        Node* oldTail = tail;
        tail = tail->prev;

        if (tail) tail->next = nullptr;
        else head = nullptr;

        delete oldTail;
        --sze;
    }

    // ham xoa phan tu co gia tri bang value dau tien trong danh sach lien ket
    void remove(const T& value) {
        Node* current = head;

        while (current && !(current->data == value)) {
            current = current->next;
        }

        if (!current) return;

        if (current == head) {
            removeFront();
            return;
        }

        if (current == tail) {
            removeBack();
            return;
        }

        current->prev->next = current->next;
        current->next->prev = current->prev;

        delete current;
        --sze;
    }

    // ham xoa phan tu o vi tri index trong danh sach lien ket
    void removeAt(std::size_t index) {
        if (index >= sze) throw std::out_of_range("Index out of range.");

        if (index == 0) {
            removeFront();
            return;
        }

        if (index == sze - 1) {
            removeBack();
            return;
        }

        Node* current = getNode(index);

        current->prev->next = current->next;
        current->next->prev = current->prev;

        delete current;
        --sze;
    }

    // ham tim kiem index cua phan tu co gia tri bang value trong danh sach lien ket, tra ve npos neu khong tim thay
    std::size_t find(const T& value) const {
        const Node* current = head;
        std::size_t index = 0;

        while (current) {
            if (current->data == value) {
                return index;
            }

            current = current->next;
            ++index;
        }

        return npos;
    }

    // ham truy cap phan tu o dau danh sach lien ket
    T& front() {
        if (empty()) throw std::out_of_range("List is empty.");

        return head->data;
    }

    // ham truy cap phan tu o dau danh sach lien ket, khong thay doi du lieu
    const T& front() const {
        if (empty()) throw std::out_of_range("List is empty.");

        return head->data;
    }

    // ham truy cap phan tu o cuoi danh sach lien ket
    T& back() {
        if (empty()) throw std::out_of_range("List is empty.");

        return tail->data;
    }

    // ham truy cap phan tu o cuoi danh sach lien ket, khong thay doi du lieu
    const T& back() const {
        if (empty()) throw std::out_of_range("List is empty.");

        return tail->data;
    }

    // ham truy cap phan tu o vi tri index trong danh sach lien ket
    T& get(std::size_t index) {
        if (index >= sze) throw std::out_of_range("Index out of range.");

        return getNode(index)->data;
    }

    // ham truy cap phan tu o vi tri index trong danh sach lien ket, khong thay doi du lieu
    const T& get(std::size_t index) const {
        if (index >= sze) throw std::out_of_range("Index out of range.");

        return getNode(index)->data;
    }

    // ham tra ve kich thuoc cua danh sach lien ket
    std::size_t size() const {
        return sze;
    }

    // ham tra ve true neu danh sach lien ket rong, nguoc lai tra ve false
    bool empty() const {
        return sze == 0;
    }

    // ham xoa tat ca phan tu trong danh sach lien ket
    void clear() noexcept {
        Node* current = head;

        while (current) {
            Node* nextNode = current->next;
            delete current;
            current = nextNode;
        }

        head = nullptr;
        tail = nullptr;
        sze = 0;
    }

    // ham in ra cac phan tu trong danh sach lien ket theo thu tu tu dau den cuoi
    void printForward() const {
        const Node* current = head;

        while (current) {
            std::cout << current->data << ' ';
            current = current->next;
        }

        std::cout << '\n';
    }

    // ham in ra cac phan tu trong danh sach lien ket theo thu tu tu cuoi den dau
    void printBackward() const {
        const Node* current = tail;

        while (current) {
            std::cout << current->data << ' ';
            current = current->prev;
        }

        std::cout << '\n';
    }

    // ham thuc thi mot ham tren tung phan tu trong danh sach lien ket theo thu tu tu dau den cuoi
    void forEach(
        const std::function<void(const T&)>& function
    ) const {
        const Node* current = head;

        while (current) {
            function(current->data);
            current = current->next;
        }
    }

    // ham thuc thi mot ham tren tung phan tu trong danh sach lien ket theo thu tu tu cuoi den dau
    void forEachReverse(
        const std::function<void(const T&)>& function
    ) const {
        const Node* current = tail;

        while (current) {
            function(current->data);
            current = current->prev;
        }
    }
};

}

#endif
