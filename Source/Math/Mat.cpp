#include "Mat.h"

#include "NMath.h"

vec4 LinearCombination(vec4 v, mat4 m) {
    vec4 result = {};

    result.Ew = f32x4(v.x) * m[0].Ew;
    result.Ew += f32x4(v.y) * m[1].Ew;
    result.Ew += f32x4(v.z) * m[2].Ew;
    result.Ew += f32x4(v.w) * m[3].Ew;

    /*
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
    result.w += v.w * m[3].w;*/

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

mat4 Ortho(float left, float right, float bottom, float top, float near, float far) {
    mat4 result = {};

    result[0][0] = 2.0f / (right - left);
    result[1][1] = -2.0f / (top - bottom);
    result[2][2] = -1.0f / (far - near);
    result[3][0] = -(right + left) / (right - left);
    result[3][1] = -(top + bottom) / (top - bottom);
    result[3][2] = -near / (far - near);
    result[3][3] = 1.0f;

    return result;
}

mat4 Perspective(float fov, float aspect, float near, float far) {
    mat4 result = {};

    float thfov = Tan(fov / 2.0f);

    result[0][0] = 1.0f / (aspect * thfov);
    result[1][1] = -1.0f / thfov;
    result[2][2] = far / (near - far);
    result[2][3] = -1.0f;
    result[3][2] = (near * far) / (near - far);

    return result;
}

mat4 LookAt(vec3 eye, vec3 center, vec3 up) {
    mat4 result(1.0f);

    vec3 f = Normalize(center - eye);
    vec3 s = Normalize(Cross(f, up));
    vec3 u = Cross(s, f);

    result[0][0] = s.x;
    result[1][0] = s.y;
    result[2][0] = s.z;
    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    result[0][2] = -f.x;
    result[1][2] = -f.y;
    result[2][2] = -f.z;
    result[3][0] = -Dot(s, eye);
    result[3][1] = -Dot(u, eye);
    result[3][2] = Dot(f, eye);

    return result;
}

mat4 Scale(mat4 m, vec3 v) {
    mat4 result = m;

    result[0][0] *= v.x;
    result[1][0] *= v.x;
    result[2][0] *= v.x;
    result[3][0] *= v.x;

    result[0][1] *= v.y;
    result[1][1] *= v.y;
    result[2][1] *= v.y;
    result[3][1] *= v.y;

    result[0][2] *= v.z;
    result[1][2] *= v.z;
    result[2][2] *= v.z;
    result[3][2] *= v.z;

    return result;
}

mat4 Translate(mat4 m, vec3 v) {
    mat4 result = m;

    result[0][3] = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3];
    result[1][3] = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3];
    result[2][3] = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3];
    result[3][3] = m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3];

    return result;
}

mat4 Rotate(mat4 m, float angle, vec3 v) {
    mat4 result;

    float c = Cos(angle);
    float s = Sin(angle);

    vec3 axis = Normalize(v);
    vec3 temp = (1.0f - c) * axis;

    mat4 rotation = {};
    rotation[0][0] = c + temp.x * axis.x;
    rotation[0][1] = temp.x * axis.y + s * axis.z;
    rotation[0][2] = temp.x * axis.z - s * axis.y;

    rotation[1][0] = temp.y * axis.x - s * axis.z;
    rotation[1][1] = c + temp.y * axis.y;
    rotation[1][2] = temp.y * axis.z + s * axis.x;

    rotation[2][0] = temp.z * axis.x + s * axis.z;
    rotation[2][1] = temp.z * axis.z - s * axis.x;
    rotation[2][2] = c + temp.z * axis.z;

    result = rotation * m;

    return result;
}

mat4 Transpose(mat4 m) {
    mat4 result = {};

    result[0][0] = m[0][0];
    result[0][1] = m[1][0];
    result[0][2] = m[2][0];
    result[0][3] = m[3][0];
    result[1][0] = m[0][1];
    result[1][1] = m[1][1];
    result[1][2] = m[2][1];
    result[1][3] = m[3][1];
    result[2][0] = m[0][2];
    result[2][1] = m[1][2];
    result[2][2] = m[2][2];
    result[2][3] = m[3][2];
    result[3][0] = m[0][3];
    result[3][1] = m[1][3];
    result[3][2] = m[2][3];
    result[3][3] = m[3][3];

    return result;
}

mat4 Inverse(mat4 mat) {
    mat4 result = {};

    float   a = mat[0][0], b = mat[0][1], c = mat[0][2], d = mat[0][3],
			e = mat[1][0], f = mat[1][1], g = mat[1][2], h = mat[1][3],
			i = mat[2][0], j = mat[2][1], k = mat[2][2], l = mat[2][3],
			m = mat[3][0], n = mat[3][1], o = mat[3][2], p = mat[3][3];

    float c1 = k * p - l * o, c2 = c * h - d * g, c3 = i * p - l * m;
    float c4 = a * h - d * e, c5 = j * p - l * n, c6 = b * h - d * f;
    float c7 = i * n - j * m, c8 = a * f - b * e, c9 = j * o - k * n;
    float c10 = b * g - c * f, c11 = i * o - k * m, c12 = a * g - c * e;

    float idt = 1.0f / (c8 * c1 + c4 * c9 + c10 * c3 + c2 * c7 - c12 * c5 - c6 * c11);
    float ndt = -idt;

    result[0][0] = (f * c1 - g * c5 + h * c9) * idt;
    result[0][1] = (b * c1 - c * c5 + d * c9) * ndt;
    result[0][2] = (n * c2 - o * c6 + p * c10) * idt;
    result[0][3] = (j * c2 - k * c6 + l * c10) * ndt;

    result[1][0] = (e * c1 - g * c3 + h * c11) * ndt;
    result[1][1] = (a * c1 - c * c3 + d * c11) * idt;
    result[1][2] = (m * c2 - o * c4 + p * c12) * ndt;
    result[1][3] = (i * c2 - k * c4 + l * c12) * idt;

    result[2][0] = (e * c5 - f * c3 + h * c7) * idt;
    result[2][1] = (a * c5 - b * c3 + d * c7) * ndt;
    result[2][2] = (m * c6 - n * c4 + p * c8) * idt;
    result[2][3] = (i * c6 - j * c4 + l * c8) * ndt;

    result[3][0] = (e * c9 - f * c11 + g * c7) * ndt;
    result[3][1] = (a * c9 - b * c11 + c * c7) * idt;
    result[3][2] = (m * c10 - n * c12 + o * c8) * ndt;
    result[3][3] = (i * c10 - j * c12 + k * c8) * idt;

    return result;
}

void ExtractFrustumPlanes(mat4 proj, vec4 dest[6]) {
    mat4 t = Transpose(proj);

	dest[0] = Normalize(t[3] + t[0]); // left
	dest[1] = Normalize(t[3] - t[0]); // right
	dest[2] = Normalize(t[3] + t[1]); // top
	dest[3] = Normalize(t[3] - t[1]); // bottom
	dest[4] = Normalize(t[3] + t[2]); // near
	dest[5] = Normalize(t[3] - t[2]); // far
}

void PrintMat(mat4 m) {
    Print("| %.2f, %.2f, %.2f, %.2f |\n", m[0][0], m[0][1], m[0][2], m[0][3]);
    Print("| %.2f, %.2f, %.2f, %.2f |\n", m[1][0], m[1][1], m[1][2], m[1][3]);
    Print("| %.2f, %.2f, %.2f, %.2f |\n", m[2][0], m[2][1], m[2][2], m[2][3]);
    Print("| %.2f, %.2f, %.2f, %.2f |\n\n", m[3][0], m[3][1], m[3][2], m[3][3]);
}
