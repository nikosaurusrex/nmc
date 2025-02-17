#version 450

#extension GL_GOOGLE_include_directive: require

#include "Common.h"

layout(location=0) in vec2 p_uv;

layout(location=0) out vec4 color;

layout(set=0, binding=0) uniform sampler2D s_tex;

#define EXPOSURE 1
#define BLOOM_THRESHOLD 1.0
#define BLOOM_INTENSITY 0.2
#define VIGNETTE_INTENSITY 0.5
#define BRIGHTNESS 0.9
#define CONTRAST 1.0
#define SATURATION 1.2

vec3 ToneMapACES(vec3 col) {
    col = max(vec3(0.0), col - 0.004);
    col = (col * (6.2 * col + 0.5)) / (col * (6.2 * col + 1.7) + 0.06);
    return col;
}

vec3 ApplyBloom(vec2 uv) {
    vec3 bloom = vec3(0.0);
    float count = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(float(x), float(y)) / textureSize(s_tex, 0);
            vec3 sampled = texture(s_tex, uv + offset).rgb;
            if (length(sampled) > BLOOM_THRESHOLD) {
                bloom += sampled;
                count += 1.0;
            }
        }
    }
    if (count > 0.0)
        bloom = (bloom / count) * BLOOM_INTENSITY;
    return bloom;
}

vec3 Vignette(vec3 col, vec2 uv) {
    float dist = distance(uv, vec2(0.5));
    float vig = smoothstep(0.8, 0.5, dist);
    return col * mix(1.0, vig, VIGNETTE_INTENSITY);
}

vec3 ColorCorrect(vec3 col) {
    col *= BRIGHTNESS;
    col = ((col - 0.5) * CONTRAST) + 0.5;
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, SATURATION);
    return col;
}

vec3 ApplyFXAA(vec2 uv) {
    vec2 inv_res = 1.0 / textureSize(s_tex, 0);
    vec3 col = texture(s_tex, uv).rgb;
    vec3 n = texture(s_tex, uv + vec2(0.0, inv_res.y)).rgb;
    vec3 s = texture(s_tex, uv - vec2(0.0, inv_res.y)).rgb;
    vec3 e = texture(s_tex, uv + vec2(inv_res.x, 0.0)).rgb;
    vec3 w = texture(s_tex, uv - vec2(inv_res.x, 0.0)).rgb;
    return (col + n + s + e + w) / 5.0;
}

void main() {
    vec3 tex = texture(s_tex, p_uv).rgb;

    vec3 ldr = tex;
    ldr = ToneMapACES(ldr);

    vec3 bloom = ApplyBloom(p_uv);
    ldr = ldr + bloom;

    ldr = Vignette(ldr, p_uv);
    ldr = ColorCorrect(ldr);
    ldr *= EXPOSURE;

    vec3 aa = ApplyFXAA(p_uv);
    ldr = mix(ldr, aa, 0.5);

    ldr = clamp(ldr, 0.0, 1.0);

    color = vec4(ldr, 1.0);
}
