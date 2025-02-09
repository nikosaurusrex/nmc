#pragma once

#include "../General.h"

#if ARCH_X64
#include <immintrin.h>
#endif

union f32x4 {
    float E[4];

#if ARCH_X64
    __m128 SSE;
#endif

    f32x4();
    f32x4(float value);
    f32x4(float x, float y, float z, float w);
    f32x4(float *address);
};

void Store(f32x4 v, float *address);

f32x4 operator+(f32x4 a, f32x4 b);
f32x4 operator-(f32x4 a, f32x4 b);
f32x4 operator*(f32x4 a, f32x4 b);
f32x4 operator/(f32x4 a, f32x4 b);

f32x4 operator-(f32x4 a);

f32x4 &operator+=(f32x4 &a, f32x4 b);
f32x4 &operator-=(f32x4 &a, f32x4 b);
f32x4 &operator*=(f32x4 &a, f32x4 b);
f32x4 &operator/=(f32x4 &a, f32x4 b);

f32x4 operator<(f32x4 a, f32x4 b);
f32x4 operator<=(f32x4 a, f32x4 b);
f32x4 operator>(f32x4 a, f32x4 b);
f32x4 operator>=(f32x4 a, f32x4 b);
f32x4 operator==(f32x4 a, f32x4 b);
f32x4 operator!=(f32x4 a, f32x4 b);

f32x4 operator&(f32x4 a, f32x4 b);
f32x4 operator|(f32x4 a, f32x4 b);

f32x4 &operator&=(f32x4 &a, f32x4 b);
f32x4 &operator|=(f32x4 &a, f32x4 b);

f32x4 Minimum(f32x4 a, f32x4 b);
f32x4 Maximum(f32x4 a, f32x4 b);

float HorizontalAdd(f32x4 a);

float LowestFloat(f32x4 a);

b32 AnyTrue(f32x4 cond);
b32 AllTrue(f32x4 cond);
b32 AllFalse(f32x4 cond);
