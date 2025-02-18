#version 450

#extension GL_EXT_nonuniform_qualifier: require
#extension GL_GOOGLE_include_directive: require

#include "Common.h"

layout(set=0, binding=0) uniform GlobalsUniform {
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 light_space_matrix;
    vec3 camera_pos;
};

layout(set=0, binding=2) uniform sampler2D s_shadow;
layout(set=0, binding=3) uniform sampler2D s_noise;
layout(set=0, binding=4) uniform sampler2D s_water1;
layout(set=0, binding=5) uniform sampler2D s_water2;

layout(push_constant, std430) uniform time_pc {
    float time;
};

layout(location=0) in vec3 p_world_pos;
layout(location=1) in vec3 p_normal;
layout(location=2) in vec3 p_tangent;
layout(location=3) in vec3 p_bitangent;
layout(location=4) in vec2 p_uv;
layout(location=5) in vec4 p_shadow_pos;

layout(location=0) out vec4 color;

#define WAVE_SCALE 1
#define RIPPLE_SCALE 0.1

void main() {
    vec2 world_uv = p_world_pos.xz * RIPPLE_SCALE;

    vec2 uv1 = world_uv + vec2(0.05, 0.025) * time;
    vec2 uv2 = world_uv - vec2(0.035, 0.015) * time;
    vec3 normal1 = texture(s_water1, uv1).xyz * 2.0 - 1.0;
    vec3 normal2 = texture(s_water2, uv2).xyz * 2.0 - 1.0;

    vec3 blended_normal = normalize(mix(normal1, normal2, 0.5));

    vec3 T = normalize(p_tangent);
    vec3 B = normalize(p_bitangent);
    vec3 N = normalize(p_normal);
    mat3 TBN = mat3(T, B, N);

    vec3 world_normal = normalize(TBN * blended_normal);
    world_normal = normalize(N + world_normal * WAVE_SCALE);

    N = world_normal;
    vec3 V = normalize(camera_pos - p_world_pos);
    vec3 L = sun_light.direction;
    vec3 H = normalize(V + L);

    float roughness = 0.05;
    float metallic = 0;
    float F0 = 0.02;

    float fresnel = FresnelSchlick(max(dot(V, N), 0.0), F0);
    vec3 water_color = mix(vec3(0.07, 0.3, 0.7), vec3(0.009, 0.15, 0.8), fresnel);

    float shadow = ShadowCalculation(p_shadow_pos, N, s_shadow, s_noise);
    vec3 lighting = CalculatePBR(N, V, L, H, roughness, metallic, F0, water_color, shadow);

    float transparency = 0.9 + fresnel * 0.1;

    color = vec4(lighting, transparency);
}
