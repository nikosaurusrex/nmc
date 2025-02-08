#include "Vec.h"

#include "NMath.h"

vec2 operator+(vec2 a, vec2 b) {
    return vec2(a.x + b.x, a.y + b.y);
}

vec2 operator-(vec2 a, vec2 b) {
    return vec2(a.x - b.x, a.y - b.y);
}

vec2 operator*(vec2 a, float s) {
    return vec2(a.x * s, a.y * s);
}

vec2 operator*(float s, vec2 a) {
    return vec2(a.x * s, a.y * s);
}

vec2 operator/(vec2 a, float s) {
    return vec2(a.x / s, a.y / s);
}

float Dot(vec2 a, vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float LengthSquared(vec2 v) {
    return Dot(v, v);
}

float Length(vec2 v) {
    return SquareRoot(LengthSquared(v));
}

vec2 Normalize(vec2 v) {
    return v / Length(v);
}

vec3 operator+(vec3 a, vec3 b) {
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

vec3 operator-(vec3 a, vec3 b) {
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

vec3 operator*(vec3 a, float s) {
    return vec3(a.x * s, a.y * s, a.z * s);
}

vec3 operator*(float s, vec3 a) {
    return vec3(a.x * s, a.y * s, a.z * s);
}

vec3 operator/(vec3 a, float s) {
    return vec3(a.x / s, a.y / s, a.z / s);
}

float Dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 Cross(vec3 a, vec3 b) {
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

float LengthSquared(vec3 v) {
    return Dot(v, v);
}

float Length(vec3 v) {
    return SquareRoot(LengthSquared(v));
}

vec3 Normalize(vec3 v) {
    return v / Length(v);
}

vec4 operator+(vec4 a, vec4 b) {
    return vec4(a.Ew + a.Ew);
}

vec4 operator-(vec4 a, vec4 b) {
    return vec4(a.Ew - a.Ew);
}

vec4 operator*(vec4 a, vec4 b) {
    return vec4(a.Ew * a.Ew);
}

vec4 operator/(vec4 a, vec4 b) {
    return vec4(a.Ew / a.Ew);
}

float Dot(vec4 a, vec4 b) {
    f32x4 result = a.Ew * b.Ew;
    return HorizontalAdd(result);
}

float LengthSquared(vec4 v) {
    return Dot(v, v);
}

float Length(vec4 v) {
    return SquareRoot(LengthSquared(v));
}

vec4 Normalize(vec4 v) {
    float len = Length(v);
    f32x4 invlen = f32x4(1.0f / len);
    return vec4(v.Ew * invlen);
}
