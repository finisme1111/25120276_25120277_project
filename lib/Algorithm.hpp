#ifndef ALGORITHM_HPP
#define ALGORITHM_HPP

#include <cstddef>
#include <functional>
#include <utility>

namespace lib {
    // gia tri tra ve neu khong tim thay
    inline constexpr std::size_t npos =
        static_cast<std::size_t>(-1);

    // ham swap: hoan doi gia tri cua 2 bien
    template <typename T>
    void swap(T& a, T& b) {
        T temp = std::move(a);
        a = std::move(b);
        b = std::move(temp);
    }

    // LINEAR SEARCH
    // ham linear_search: tim kiem tuan tu
    template <typename T>
    std::size_t linear_search(const T a[], std::size_t n, const T& value) {
        if (a == nullptr) return npos;

        for (std::size_t i = 0; i < n; ++i) {
            if (a[i] == value) return i;
        }
        return npos;
    }

    // BINARY SEARCH
    // ham binary_search: tim kiem nhi phan, mang phai duoc sap xep truoc 
    template <typename T, typename Comp = std::less<T>>
    std::size_t binary_search(const T a[], std::size_t n, const T& value, Comp cmp = Comp{}) {
        if (a == nullptr) return npos;
        std::size_t left = 0;
        std::size_t right = n;

        while (left < right) {
            const std::size_t mid =
                left + (right - left) / 2;
            if (cmp(a[mid], value)) left = mid + 1;
            else if (cmp(value, a[mid])) right = mid;
            else return mid;
        }
        return npos;
    }

    // SELECTION SORT
    // ham selection_sort: sap xep chon
    template <typename T, typename Comp = std::less<T>>
    void selection_sort(T a[], std::size_t n, Comp cmp = Comp{}) {
        if (a == nullptr || n < 2) return;

        for (std::size_t i = 0; i + 1 < n; ++i) {
            std::size_t minIdx = i;

            for (std::size_t j = i + 1; j < n; ++j) 
                if (cmp(a[j], a[minIdx])) minIdx = j;

            if (minIdx != i) swap(a[i], a[minIdx]);
        }
    }

    // INSERTION SORT
    // ham insertion_sort: sap xep chen
    template <typename T, typename Comp = std::less<T>>
    void insertion_sort(T a[], std::size_t n, Comp cmp = Comp{}) {
        if (a == nullptr || n < 2) return;

        for (std::size_t i = 1; i < n; ++i) {
            T key = std::move(a[i]);
            std::size_t j = i;

            while (j > 0 && cmp(key, a[j - 1])) {
                a[j] = std::move(a[j - 1]);
                --j;
            }

            a[j] = std::move(key);
        }
    }

    // BUBBLE SORT
    // ham bubble_sort: sap xep noi bot
    template <typename T, typename Comp = std::less<T>>
    void bubble_sort(T a[], std::size_t n, Comp cmp = Comp{}) {
        if (a == nullptr || n < 2) return;

        for (std::size_t i = 0; i < n; ++i) {
            bool swapped = false;

            for (std::size_t j = 0; j < n - i - 1; ++j) {
                if (cmp(a[j + 1], a[j])) {
                    swap(a[j], a[j + 1]);
                    swapped = true;
                }
            }

            if (!swapped) break;
        }
    }

    // HEAP SORT: sap xep vun dong
    // ham heapify: vun dong mot cay con co goc la node i
    template <typename T, typename Comp = std::less<T>>
    void heapify(T a[], std::size_t n, std::size_t i, Comp cmp = Comp{}) {
        if (a == nullptr || n < 2 || i >= n) return;

        while (i < n / 2) {
            std::size_t largest = i;

            const std::size_t left = 2 * i + 1;
            const std::size_t right = left + 1;

            if (left < n && cmp(a[largest], a[left])) largest = left;

            if (right < n && cmp(a[largest], a[right])) largest = right;

            if (largest == i) return;

            swap(a[i], a[largest]);
            i = largest;
        }
    }

    // ham heap_sort: sap xep vun dong
    template <typename T, typename Comp = std::less<T>>
    void heap_sort(T a[], std::size_t n, Comp cmp = Comp{}) {
        if (a == nullptr || n < 2) return;

        for (std::size_t i = n / 2; i > 0; --i) heapify(a, n, i - 1, cmp);

        for (std::size_t i = n - 1; i > 0; --i) {
            swap(a[0], a[i]);
            heapify(a, i, 0, cmp);
        }
    }

    // MERGE SORT
    // ham merge: gop 2 doan con da sap xep thanh 1 doan da sap xep
    template <typename T, typename Comp = std::less<T>>
    void merge(T a[], std::size_t left, std::size_t middle, std::size_t right, Comp cmp = Comp{}) {
        if (a == nullptr || right == npos || left > middle || middle >= right) return;

        const std::size_t size = right - left + 1;

        T* temp = new T[size];

        std::size_t i = left;
        std::size_t j = middle + 1;
        std::size_t k = 0;

        try {
            while (i <= middle && j <= right) {
                if (!cmp(a[j], a[i])) {
                    temp[k++] = a[i++];
                } else {
                    temp[k++] = a[j++];
                }
            }

            while (i <= middle) {
                temp[k++] = a[i++];
            }

            while (j <= right) {
                temp[k++] = a[j++];
            }

            for (std::size_t x = 0; x < size; ++x) {
                a[left + x] = std::move(temp[x]);
            }
        } catch (...) {
            delete[] temp;
            throw;
        }

        delete[] temp;
    }

    // ham merge_sort: sap xep tron
    template <typename T, typename Comp = std::less<T>>
    void merge_sort(T a[], std::size_t left, std::size_t right, Comp cmp = Comp{}) {
        if (a == nullptr || right == npos || left >= right) return;

        const std::size_t mid = left + (right - left) / 2;

        merge_sort(a, left, mid, cmp);
        merge_sort(a, mid + 1, right, cmp);
        merge(a, left, mid, right, cmp);
    }

    // QUICK SORT
    // ham hoare partition: phan hoach theo hoare, tra ve chi so cua phan tu cuoi cung cua doan con trai
    template <typename T, typename Comp = std::less<T>>
    std::size_t partition(T a[], std::size_t left, std::size_t right, Comp cmp = Comp{}) {
        const T pivot = a[left + (right - left) / 2];

        std::size_t i = left;
        std::size_t j = right;

        while (true) {
            while (cmp(a[i], pivot)) ++i;

            while (cmp(pivot, a[j])) --j;

            if (i >= j) return j;

            swap(a[i], a[j]);

            ++i;
            --j;
        }
    }

    // ham quick_sort: sap xep nhanh
    template <typename T, typename Comp = std::less<T>>
    void quick_sort(T a[], std::size_t left, std::size_t right, Comp cmp = Comp{}) {
        if (a == nullptr || right == npos || left >= right) return;

        while (left < right) {
            const std::size_t pi = partition(a, left, right, cmp);

            if (pi - left < right - pi) {
                if (left < pi) quick_sort(a, left, pi, cmp); 

                left = pi + 1;
            } else {
                if (pi + 1 < right) quick_sort(a, pi + 1, right, cmp);

                right = pi;
            }
        }
    }
}

#endif