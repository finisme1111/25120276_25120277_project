#pragma once
#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP
#include <stdexcept>
#include <cstddef>
#include <functional>

namespace lib {
    template <typename T>
    void swap(T& a, T& b) {
        T tmp = a;
        a = b;
        b = tmp;
    }
    template <typename T>
    size_t linear_search(const T a[], size_t n, const T& value) {
        for (size_t i = 0; i < n; i++) {
            if (a[i] == value) {
                return i;
            }
        }
        return -1;
    }

    template <typename T>
    size_t binary_search(const T a[], size_t n, const T& value) {
        size_t l = 0;
        size_t r = n - 1;
        while (l <= r) {
            size_t m = l + (r - l) / 2;
            if (a[m] = value) return m;
            else if (a[m] > value) r = m - 1;
            else l = m + 1;
        }
        return -1;
    }

    template <typename T, typename Comp = std::less<T>>
    void selection_sort(T a[], size_t n, Comp cmp = std::less<T>()) {
        for (size_t i = 0; i < n - 1; i++) {
            size_t min_idx = i;
            for (size_t j = i + 1; j < n; j++) {
                if (cmp(a[j], a[min_idx])) min_idx = j;
            }
            swap(a[i], a[min_idx]);
        }
    }

    template <typename T, typename Comp = std::less<T>>
    void insertion_sort(T a[], size_t n, Comp cmp = std::less<T>()) {
        for (size_t i = 1; i < n; i++) {
            T key = a[i];
            size_t j = i - 1;
            while (j >= 0 && cmp(key, a[j])) {
                a[j + 1] = a[j];
                j--;
            }
            a[j + 1] = key;
        }
    } 

    template <typename T, typename Comp = std::less<T>>
    void bubble_sort(T a[], size_t n, Comp cmp = std::less<T>()) {
        for (size_t i = 0; i < n - 1; i++) {
            for (size_t j = 0; j < n - i - 1; j++) {
                if (cmp(a[j + 1], a[j])) swap(a[j + 1], a[j]);
            }
        }
    }

    template <typename T, typename Comp = std::less<T>>
    void heapify(T a[], size_t n, size_t i, Comp cmp = std::less<T>()) {
        size_t largest = i;
        size_t l = 2 * i + 1;
        size_t r = 2 * i + 2;
        if (l < n && cmp(a[largest], a[l])) largest = l;
        if (r < n && cmp(a[largest], a[r])) largest = r;
        if (largest != i) {
            swap(a[i], a[largest]);
            heapify(a, n, largest, cmp);
        }
    }

    template <typename T, typename Comp = std::less<T>>
    void heap_sort(T a[], size_t n, Comp cmp = std::less<T>()) {
        for (size_t i = n / 2 - 1; i >= 0 ;i--) {
            heapify(a, n, i, cmp);
        }
        for (size_t i = n - 1; i > 0; i--) {
            swap(a[0], a[i]);
            heapify(a, i, 0, cmp);
        }
    }

    template <typename T, typename Comp = std::less<T>>
    void merge(T a[], size_t l, size_t m, size_t r, Comp cmp = std::less<T>()) {
        size_t i = l;
        size_t j = m + 1;
        size_t k = 0;
        T *res = new T[r - l + 1];
        while (i <= m && j <= r) {
            if (cmp(a[i], a[j])) res[k++] = a[i++];
            else res[k++] = a[j++];
        }
        while (i <= m) res[k++] = a[i++];
        while (j <= r) res[k++] = a[j++];
        for (size_t x = 0; x < r - l + 1; x++) a[l + x] = res[x];
        delete[] res;
    }

    template <typename T, typename Comp = std::less<T>>
    void merge_sort(T a[], size_t l, size_t r, Comp cmp = std::less<T>()) {
        if (r - l + 1 <= 1) return;
        size_t m = l + (r - l) / 2;
        merge_sort(a, l, m, cmp);
        merge_sort(a, m + 1, r, cmp);
        merge(a, l, m, r, cmp);
    }

    template <typename T, typename Comp = std::less<T>>
    size_t partition(T a[], size_t l, size_t r, Comp cmp = std::less<T>()) {
        T pivot = a[l];
        size_t i = l - 1;
        size_t j = r + 1;
        while (true) {
            do{
                i++;
            } while (cmp(a[i], pivot));
            do {
                j--;
            } while (cmp(pivot, a[j]));     
            if (i >= j) return j;
            swap(a[i], a[j]);
        }
    }

    template <typename T, typename Comp = std::less<T>>
    void quick_sort(T a[], size_t l, size_t r, Comp cmp = std::less<T>()) {
        if (l < r) {
            size_t pi = partition(a, l, r, cmp);
            quick_sort(a, l, pi, cmp);
            quick_sort(a, pi + 1, r, cmp);
        }
    }
}

#endif