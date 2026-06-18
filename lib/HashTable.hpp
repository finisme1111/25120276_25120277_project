#pragma once
#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include "AVL.hpp"

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>

namespace lib {

template <typename KeyType, typename ValueType>
class HashTable {
private:
    using Pair = std::pair<KeyType, ValueType>;

    // so sanh 2 pair chi duoc so sanh theo key
    struct PairCompare {
        bool operator()(
            const Pair& first,
            const Pair& second
        ) const {
            return std::less<KeyType>{}(
                first.first,
                second.first
            );
        }
    };

    // ham lay key tu pair
    struct KeyExtractor {
        const KeyType& operator()(const Pair& item) const {
            return item.first;
        }
    };

    // moi bucket la 1 AVL rieng biet, sap xep theo key cua pair
    using Bucket = AVL<Pair, PairCompare>;

    Bucket* table;
    std::size_t capacity;
    std::size_t currentSize;

    // ham bam key de tim index cua bucket
    std::size_t hashFunction(const KeyType& key) const {
        return std::hash<KeyType>{}(key) % capacity;
    }

public:
    // khoi tao bang bam voi so bucket mac dinh la 101
    explicit HashTable(std::size_t cap = 101): table(nullptr), capacity(cap), currentSize(0) {
        if (capacity == 0) throw std::invalid_argument("Hash table capacity must be greater than zero.");

        // moi phan tu cua table la 1 AVL rieng biet
        table = new Bucket[capacity];
    }

    // xoa bang bam va giai phong bo nho
    ~HashTable() {
        delete[] table;
    }

    // vo hieu hoa ham sao chep de tranh sao chep khong mong muon
    HashTable(const HashTable&) = delete;

    // vo hieu hoa toan tu gan de tranh gan khong mong muon
    HashTable& operator=(const HashTable&) = delete;

    // ham them key-value moi vao bang bam
    void insert(const KeyType& key, const ValueType& value) {
        const std::size_t index = hashFunction(key);
        Bucket& bucket = table[index];

        Pair* existing = bucket.findByKey(key, KeyExtractor{});

        if (existing) {
            existing->second = value;
            return;
        }

        if (bucket.insert(Pair(key, value))) ++currentSize;
    }

    // ham xoa phan tu theo key
    bool remove(const KeyType& key) {
        const std::size_t index = hashFunction(key);
        Bucket& bucket = table[index];

        Pair* item = bucket.findByKey(key, KeyExtractor{});

        if (!item) return false;

        Pair target = *item;

        if (bucket.remove(target)) {
            --currentSize;
            return true;
        }

        return false;
    }

    // ham kiem tra key co ton tai hay khong
    bool contains(const KeyType& key) const {
        return find(key) != nullptr;
    }

    // ham tim kiem value theo key, tra ve con tro den value de co the chinh sua
    ValueType* find(const KeyType& key) {
        const std::size_t index = hashFunction(key);
        Bucket& bucket = table[index];

        Pair* item = bucket.findByKey(
            key,
            KeyExtractor{}
        );

        return item ? &item->second : nullptr;
    }

    // ham tim kiem value theo key, tra ve con tro den value de chi doc
    const ValueType* find(const KeyType& key) const {
        const std::size_t index = hashFunction(key);

        const Bucket& bucket = table[index];

        const Pair* item = bucket.findByKey(
            key,
            KeyExtractor{}
        );

        return item ? &item->second : nullptr;
    }

    // ham xoa toan bo phan tu trong bang bam, nhung van giu lai cac bucket (AVL) de su dung lai
    void clear() {
        for (std::size_t i = 0; i < capacity; ++i) {
            table[i].clear();
        }

        currentSize = 0;
    }

    // ham lay so luong phan tu hien tai trong bang bam
    std::size_t size() const {
        return currentSize;
    }

    bool empty() const {
        return currentSize == 0;
    }

    // ham duyet toan bo phan tu trong bang bam, truyen vao 1 ham de thuc thi tren moi cap key-value
    void forEach(const std::function<void(const KeyType&, const ValueType&)> & function) const {
        for (std::size_t i = 0; i < capacity; ++i) {
            const Bucket& bucket = table[i];

            bucket.inorder(
                [&function](const Pair& item) {
                    function(item.first, item.second);
                }
            );
        }
    }
};

}

#endif