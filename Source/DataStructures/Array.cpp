#include "Array.h"

template<typename T>
void Reserve(Array<T> &a, u64 count) {
    if (count == 0) {
        count = 4;
    }

    if (count <= a.cap) {
        return;
    }

    T *newptr = (T *)HeapAlloc(count * sizeof(T));

    if (a.ptr) {
        CopyMemory(newptr, a.ptr, a.len * sizeof(T));
        HeapFree(a.ptr);
    }

    a.ptr = newptr;
    a.cap = count;
}

template<typename T>
void Append(Array<T> &a, T element) {
    if (a.len + 1 >= a.cap) {
        Reserve(a, a.cap * 2);
    }

    a.ptr[a.len] = element;
    a.len++;
}

template<typename T>
void UnorderedRemove(Array<T> &a, u64 idx) {
    T last = Pop(a);

    if (idx < a.len) {
        a.ptr[idx] = last;
    }
}

template<typename T>
void OrderedRemove(Array<T> &a, u64 idx) {
    MoveMemory(a.ptr + idx, a.ptr + idx + 1, ((a.len - idx) - 1) * sizeof(T));

    a.len--;
}

template<typename T>
T Pop(Array<T> &a) {
    T result = a.ptr[a.len - 1];

    a.len--;

    return result;
}

template<typename T>
void Clear(Array<T> &a) {
    a.len = 0;
}

template<typename T>
void Free(Array<T> &a) {
    a.len = 0;
    a.cap = 0;

    if (a.ptr) {
        HeapFree(a.ptr);
        a.ptr = 0;
    }
}
