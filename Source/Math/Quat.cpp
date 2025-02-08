#include "Quat.h"

quat Invert(quat q) {
    quat result = {};

    result.w = q.w;
    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;

    return result;
}

quat operator*(quat q, quat p) {
    quat result = {};

    result.w = q.w * p.w - Dot(p.v, q.v);
    result.v = q.w * p.v + p.w * q.v + Cross(q.v, p.v);

    return result;
}

vec3 operator*(quat q, vec3 v) {
    vec3 qvXv = Cross(q.v, v);

    return v + qvXv * (q.w * 2.0f) + Cross(q.v, qvXv) * 2.0f;
}
