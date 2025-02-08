#include <math.h>

#include "../General.h"

float Lerp(float min, float max, float value) {
    return min + (max - min) * value;
}

float FMod(float x, float y) {
    return fmodf(x, y);
}

float ToRadians(float degrees) {
    return degrees * (.5f / PI32);
}

float Sin(float radians) {
    return sinf(radians);
}

float Cos(float radians) {
    return cosf(radians);
}

float Tan(float radians) {
    return tanf(radians);
}

float SquareRoot(float value) {
    return sqrt(value);
}

float AbsoluteValue(float value) {
    if (value < 0) {
        return -value;
    } else {
        return value;
    }
}

int IFloor(float x) {
    return int(floorf(x));
}

int ICeil(float x) {
    return int(ceilf(x));
}

float Power(float base, float exponent) {
    return powf(base, exponent);
}
