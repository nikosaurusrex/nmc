#pragma once

#include "../General.h"
#include "../Platform/Platform.h"
#include "Vec.h"

union mat4 {
    float M[4][4];
    vec4 columns[4];

    mat4() {
        SetMemory(M, 0, sizeof(M));
    }

    mat4(float value) {
        SetMemory(M, 0, sizeof(M));
        M[0][0] = value;
        M[1][1] = value;
        M[2][2] = value;
        M[3][3] = value;
    }

    vec4 &operator[](u32 idx) {
        return columns[idx];
    }

    const vec4 &operator[](u32 idx) const {
        return columns[idx];
    }
};

mat4 Ortho(float left, float right, float bottom, float top, float near, float far);
mat4 Perspective(float fov, float aspect, float near, float far);
mat4 LookAt(vec3 eye, vec3 center, vec3 up);
mat4 Scale(mat4 m, vec3 v);
mat4 Translate(mat4 m, vec3 v);
mat4 Rotate(mat4 m, float angle, vec3 v);
mat4 Transpose(mat4 m);

mat4 operator*(mat4 a, mat4 b);
