#pragma once

#include "../General.h"

template<typename T>
struct Array {
    T *ptr;
    u64 len;
    u64 cap;

    Array() {
        ptr = 0;
        len = 0;
        cap = 0;
    }

    Array(u64 reserve_count) {
        ptr = 0;
        cap = 0;
        len = 0;

        Reserve(*this, reserve_count);
    }

    T &operator[](u64 idx) {
        return ptr[idx];
    }

    const T operator[](u64 idx) const {
        return ptr[idx];
    }
};

template<typename T>
void Reserve(Array<T> &a, u64 count);

template<typename T>
void Append(Array<T> &a, T element);

template<typename T>
void UnorderedRemove(Array<T> &a, u64 idx);
template<typename T>
void OrderedRemove(Array<T> &a, u64 idx);

template<typename T>
T Pop(Array<T> &a);

template<typename T>
void Clear(Array<T> &a);
template<typename T>
void Free(Array<T> &a);
