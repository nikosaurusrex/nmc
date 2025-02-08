#pragma once

#include "../General.h"

inline u64 CStringLength(const char *cstr) {
    u64 len = 0;

    if (cstr) {
        while (cstr[len] != 0) {
            len++;
        }
    }

    return len;
}

struct String {
    u8 *ptr;
    u64 len;

    String() {
        ptr = 0;
        len = 0;
    }

    String(const char *cstr) {
        ptr = (u8 *) cstr;
        len = CStringLength(cstr);
    }

    String(u8 *_ptr, u64 _len) {
        ptr = _ptr;
        len = _len;
    }

    u8 operator[](u64 idx) const {
        Assert(idx < len);
        return ptr[idx];
    }

    u8 &operator[](u64 idx) {
        Assert(idx < len);
        return ptr[idx];
    }

    b8 operator==(const char *cstr) {
        if ((const char *) ptr == cstr)
            return 1;
        if (!ptr)
            return 0;
        if (!cstr)
            return 0;

        u64 cstrlen = CStringLength(cstr);
        if (cstrlen != len)
            return 0;

        for (u64 i = 0; i < len; ++i) {
            if (ptr[i] != cstr[i]) {
                return 0;
            }
        }

        return 1;
    }

    b8 operator!=(const char *cstr) {
        return !operator==(cstr);
    }

    b8 operator==(String other) {
        if (other.ptr == ptr)
            return 1;
        if (!ptr)
            return 0;
        if (!other.ptr)
            return 0;

        if (other.len != len)
            return 0;

        for (u64 i = 0; i < len; ++i) {
            if (ptr[i] != other.ptr[i]) {
                return 0;
            }
        }

        return 1;
    }

    b8 operator!=(String str) {
        return !operator==(str);
    }
};

#if OS_WINDOWS
typedef wchar_t * NativeString;
#define MakeNativeString(literal) ((wchar_t *) L##literal)
#else
typedef char * NativeString;
typedef MakeNativeString(literal) ((char *) literal)
#endif

inline b32 IsDigit(char ch) {
    return ('0' <= ch && ch <= '9');
}

inline b32 IsAlpha(char ch) {
    return ('A' <= ch && ch <= 'Z')
        || ('a' <= ch && ch <= 'z');
}
