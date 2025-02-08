#include "SIMD.h"

f32x4::f32x4() {
    SSE = _mm_setzero_ps();
}

f32x4::f32x4(float value) {
    SSE = _mm_set1_ps(value);
}

f32x4::f32x4(float x, float y, float z, float w) {
    SSE = _mm_set_ps(x, y, z, w);
}

f32x4::f32x4(float *address) {
    SSE = _mm_loadu_ps(address);
}

void Store(f32x4 v, float *address) {
    _mm_storeu_ps(address, v.SSE);
}

internal f32x4 FromSSE(__m128 SSE) {
    f32x4 result = {};

    result.SSE = SSE;

    return result;
}

f32x4 operator+(f32x4 a, f32x4 b) {
    return FromSSE(_mm_add_ps(a.SSE, b.SSE));
}

f32x4 operator-(f32x4 a, f32x4 b) {
    return FromSSE(_mm_sub_ps(a.SSE, b.SSE));
}

f32x4 operator*(f32x4 a, f32x4 b) {
    return FromSSE(_mm_mul_ps(a.SSE, b.SSE));
}

f32x4 operator/(f32x4 a, f32x4 b) {
    return FromSSE(_mm_div_ps(a.SSE, b.SSE));
}

f32x4 operator-(f32x4 a) {
    return f32x4() - a;
}

f32x4 &operator+=(f32x4 &a, f32x4 b) {
    a = a + b;

    return a;
}

f32x4 &operator-=(f32x4 &a, f32x4 b) {
    a = a - b;

    return a;
}

f32x4 &operator*=(f32x4 &a, f32x4 b) {
    a = a * b;

    return a;
}

f32x4 &operator/=(f32x4 &a, f32x4 b) {
    a = a / b;

    return a;
}

f32x4 operator<(f32x4 a, f32x4 b) {
    return FromSSE(_mm_cmplt_ps(a.SSE, b.SSE));
}

f32x4 operator<=(f32x4 a, f32x4 b) {
    return FromSSE(_mm_cmple_ps(a.SSE, b.SSE));
}

f32x4 operator>(f32x4 a, f32x4 b) {
    return FromSSE(_mm_cmpgt_ps(a.SSE, b.SSE));
}

f32x4 operator>=(f32x4 a, f32x4 b) {
    return FromSSE(_mm_cmpge_ps(a.SSE, b.SSE));
}

f32x4 operator==(f32x4 a, f32x4 b) {
    return FromSSE(_mm_cmpeq_ps(a.SSE, b.SSE));
}

f32x4 operator!=(f32x4 a, f32x4 b) {
    return FromSSE(_mm_cmpneq_ps(a.SSE, b.SSE));
}

f32x4 operator&(f32x4 a, f32x4 b) {
    return FromSSE(_mm_and_ps(a.SSE, b.SSE));
}

f32x4 operator|(f32x4 a, f32x4 b) {
    return FromSSE(_mm_or_ps(a.SSE, b.SSE));
}

f32x4 &operator&=(f32x4 &a, f32x4 b) {
    a = a & b;

    return a;
}

f32x4 &operator|=(f32x4 &a, f32x4 b) {
    a = a | b;

    return a;
}

f32x4 Minimum(f32x4 a, f32x4 b) {
    return FromSSE(_mm_min_ps(a.SSE, b.SSE));
}

f32x4 Maximum(f32x4 a, f32x4 b) {
    return FromSSE(_mm_max_ps(a.SSE, b.SSE));
}

float HorizontalAdd(f32x4 a) {
    return a.E[0] + a.E[1] + a.E[2] + a.E[3];
}

float LowestFloat(f32x4 a) {
    return _mm_cvtss_f32(a.SSE);
}

b32 AnyTrue(f32x4 cond) {
    return _mm_movemask_ps(cond.SSE);
}

b32 AllTrue(f32x4 cond) {
    return _mm_movemask_ps(cond.SSE) == 15;
}

b32 AllFalse(f32x4 cond) {
    return _mm_movemask_ps(cond.SSE) == 0;
}
