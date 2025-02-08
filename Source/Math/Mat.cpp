#include "Mat.h"

#include "NMath.h"

vec4 LinearCombination(vec4 v, mat4 m) {
    vec4 result = {};

    result.x = v.x * m[0].x;
    result.y = v.x * m[0].y;
    result.z = v.x * m[0].z;
    result.w = v.x * m[0].w;

    result.x += v.y * m[1].x;
    result.y += v.y * m[1].y;
    result.z += v.y * m[1].z;
    result.w += v.y * m[1].w;

    result.x += v.z * m[2].x;
    result.y += v.z * m[2].y;
    result.z += v.z * m[2].z;
    result.w += v.z * m[2].w;

    result.x += v.w * m[3].x;
    result.y += v.w * m[3].y;
    result.z += v.w * m[3].z;
    result.w += v.w * m[3].w;

    return result;
}

mat4 operator*(mat4 a, mat4 b) {
    mat4 result = {};

    result[0] = LinearCombination(b[0], a);
    result[1] = LinearCombination(b[1], a);
    result[2] = LinearCombination(b[2], a);
    result[3] = LinearCombination(b[3], a);

    return result;
}

mat4 Ortho(float left, float right, float bottom, float top) {
    mat4 result(1.0f);

    result.M[0][0] = 2.0f / (right - left);
    result.M[1][1] = 2.0f / (top - bottom);
    result.M[2][2] = -1.0f;
    result.M[3][0] = -(right + left) / (right - left);
    result.M[3][1] = -(top + bottom) / (top - bottom);

    return result;
}

mat4 Perspective(float fov, float aspect, float near, float far) {
    mat4 result(1.0f);

    float thfov = Tan(fov / 2.0f);

    result.M[0][0] = 1.0f / (aspect * thfov);
    result.M[1][1] = -1.0f / thfov;
    result.M[2][2] = far / (near - far);
    result.M[2][3] = -1.0f;
    result.M[3][2] = -(far * near) / (far - near);

    return result;
}

mat4 LookAt(vec3 eye, vec3 center, vec3 up) {
    mat4 result(1.0f);

    vec3 f = Normalize(center - eye);
    vec3 s = Normalize(Cross(f, up));
    vec3 u = Cross(s, f);

    result.M[0][0] = s.x;
    result.M[1][0] = s.y;
    result.M[2][0] = s.z;
    result.M[0][1] = u.x;
    result.M[1][1] = u.y;
    result.M[2][1] = u.z;
    result.M[0][2] = -f.x;
    result.M[1][2] = -f.y;
    result.M[2][2] = -f.z;
    result.M[3][0] = -Dot(s, eye);
    result.M[3][1] = -Dot(u, eye);
    result.M[3][2] = Dot(f, eye);

    return result;
}

mat4 Scale(mat4 m, vec3 v) {
    mat4 result = m;

    result.M[0][0] *= v.x;
    result.M[1][0] *= v.x;
    result.M[2][0] *= v.x;
    result.M[3][0] *= v.x;

    result.M[0][1] *= v.y;
    result.M[1][1] *= v.y;
    result.M[2][1] *= v.y;
    result.M[3][1] *= v.y;

    result.M[0][2] *= v.z;
    result.M[1][2] *= v.z;
    result.M[2][2] *= v.z;
    result.M[3][2] *= v.z;

    return result;
}

mat4 Translate(mat4 m, vec3 v) {
    mat4 result = m;

    result.M[0][3] = m.M[0][0] * v.x + m.M[0][1] * v.y + m.M[0][2] * v.z + m.M[0][3];
    result.M[1][3] = m.M[1][0] * v.x + m.M[1][1] * v.y + m.M[1][2] * v.z + m.M[1][3];
    result.M[2][3] = m.M[2][0] * v.x + m.M[2][1] * v.y + m.M[2][2] * v.z + m.M[2][3];
    result.M[3][3] = m.M[3][0] * v.x + m.M[3][1] * v.y + m.M[3][2] * v.z + m.M[3][3];

    return result;
}

mat4 Rotate(mat4 m, float angle, vec3 v) {
    mat4 result;

    float c = Cos(angle);
    float s = Sin(angle);

    vec3 axis = Normalize(v);
    vec3 temp = (1.0f - c) * axis;

    mat4 rotation = {};
    rotation.M[0][0] = c + temp.x * axis.x;
    rotation.M[0][1] = temp.x * axis.y + s * axis.z;
    rotation.M[0][2] = temp.x * axis.z - s * axis.y;

    rotation.M[1][0] = temp.y * axis.x - s * axis.z;
    rotation.M[1][1] = c + temp.y * axis.y;
    rotation.M[1][2] = temp.y * axis.z + s * axis.x;

    rotation.M[2][0] = temp.z * axis.x + s * axis.z;
    rotation.M[2][1] = temp.z * axis.z - s * axis.x;
    rotation.M[2][2] = c + temp.z * axis.z;

    result = rotation * m;

    return result;
}

mat4 Transpose(mat4 m) {
    mat4 result = {};

    result.M[0][0] = m.M[0][0];
    result.M[0][1] = m.M[1][0];
    result.M[0][2] = m.M[2][0];
    result.M[0][3] = m.M[3][0];
    result.M[1][0] = m.M[0][1];
    result.M[1][1] = m.M[1][1];
    result.M[1][2] = m.M[2][1];
    result.M[1][3] = m.M[3][1];
    result.M[2][0] = m.M[0][2];
    result.M[2][1] = m.M[1][2];
    result.M[2][2] = m.M[2][2];
    result.M[2][3] = m.M[3][2];
    result.M[3][0] = m.M[0][3];
    result.M[3][1] = m.M[1][3];
    result.M[3][2] = m.M[2][3];
    result.M[3][3] = m.M[3][3];

    return result;
}

void PrintMat(mat4 m) {
    Print("| %.2f, %.2f, %.2f, %.2f |\n", m.M[0][0], m.M[0][1], m.M[0][2], m.M[0][3]);
    Print("| %.2f, %.2f, %.2f, %.2f |\n", m.M[1][0], m.M[1][1], m.M[1][2], m.M[1][3]);
    Print("| %.2f, %.2f, %.2f, %.2f |\n", m.M[2][0], m.M[2][1], m.M[2][2], m.M[2][3]);
    Print("| %.2f, %.2f, %.2f, %.2f |\n\n", m.M[3][0], m.M[3][1], m.M[3][2], m.M[3][3]);
}
