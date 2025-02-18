#version 450

#extension GL_GOOGLE_include_directive: require

#include "Common.h"

layout(location=0) in vec2 p_uv;

layout(location=0) out vec4 color;

layout(set=0, binding=0) uniform SkyUniform {
    mat4 inv_proj;
    mat4 inv_view;
    vec3 camera_pos;
};

vec3 RenderSky(vec3 ray) {
    float t = clamp(ray.y * 0.5 + 0.5, 0.0, 1.0);
    return mix(sky_color_1, sky_color_2, t);
}

vec3 RenderSun(vec3 ray) {
    float angle = acos(dot(ray, sun_light.direction));
    float mask = smoothstep(0.11, 0.0, angle);
    return vec3(mask) * vec3(2.0, 3.0, 1.0);
}

float Noise(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898,78.233,37.719))) * 43758.5453);
}

vec3 GetRayDir(vec2 uv) {
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 clip = vec4(ndc, -1.0, 1.0);
    vec4 view = inv_proj * clip;
    view /= view.w;
    vec4 world = inv_view * vec4(view.xyz, 0.0);
    return normalize(world.xyz);
}

void main() {
    vec3 ray_dir = GetRayDir(p_uv);

    vec3 sky = RenderSky(ray_dir);
    vec3 sun = RenderSun(ray_dir);
    // vec3 clouds = RenderClouds(camera_pos, ray_dir);

    vec3 acc = sun + sky;

    color = vec4(acc, 1.0);
}
