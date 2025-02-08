#pragma once

#include "NMath.h"
#include "Vec.h"

union quat {
    struct {
        float w;
        float x;
        float y;
        float z;
    };
    struct {
        float _unusued;
        vec3 v;
    };

    quat() {
        w = .0f;
        x = .0f;
        y = .0f;
        z = .0f;
    }

    quat(vec3 n, float theta) {
        theta = theta * (PI32 / 180.0f);

        float th = theta / 2.0f;

        w = Cos(th);

        x = n.x * Sin(th);
        y = n.y * Sin(th);
        z = n.z * Sin(th);
    }
};
