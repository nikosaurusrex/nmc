#pragma once

#include "SIMD.h"

union vec2 {
    struct {
        float x;
        float y;
    };
    float E[2];

    vec2() {
        x = 0;
        y = 0;
    }

    vec2(float v) {
        x = v;
        y = v;
    }

    vec2(float _x, float _y) {
        x = _x;
        y = _y;
    }
};

vec2 operator+(vec2 a, vec2 b);
vec2 operator-(vec2 a, vec2 b);
vec2 operator*(vec2 a, float s);
vec2 operator*(float s, vec2 a);
vec2 operator/(vec2 a, float s);

float Dot(vec2 a, vec2 b);
float LengthSquared(vec2 v);
float Length(vec2 v);
vec2 Normalize(vec2 v);

union vec3 {
    struct {
        float x, y, z;
    };
    struct {
        float r, g, b;
    };
    float E[3];

    vec3() {
        x = 0;
        y = 0;
        z = 0;
    }

    vec3(float v) {
        x = v;
        y = v;
        z = v;
    }

    vec3(float _x, float _y, float _z) {
        x = _x;
        y = _y;
        z = _z;
    }
};

vec3 operator+(vec3 a, vec3 b);
vec3 operator-(vec3 a, vec3 b);
vec3 operator*(vec3 a, float s);
vec3 operator*(float s, vec3 a);
vec3 operator/(vec3 a, float s);

float Dot(vec3 a, vec3 b);
vec3 Cross(vec3 a, vec3 b);
float LengthSquared(vec3 v);
float Length(vec3 v);
vec3 Normalize(vec3 v);

union vec4 {
    struct {
        float x, y, z, w;
    };

    float E[4];
    f32x4 Ew;

    vec4() {
        Ew = f32x4();
    }

    vec4(float v) {
        Ew = f32x4(v);
    }

    vec4(float x, float y, float z, float w) {
        Ew = f32x4(w, z, y, x);
    }

    vec4(f32x4 _w) {
        Ew = _w;
    }
};
