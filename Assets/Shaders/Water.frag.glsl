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

layout(set=0, binding=2) uniform sampler2D s_water;
layout(set=0, binding=3) uniform sampler2D s_shadow;
layout(set=0, binding=4) uniform sampler2D s_noise;

layout(location=0) in vec3 p_world_pos;
layout(location=1) in vec3 p_normal;
layout(location=2) in vec2 p_uv;
layout(location=3) in vec4 p_shadow_pos;

layout(location=0) out vec4 color;

void main() {
    vec3 albedo = vec3(0.007716, 0.048228, 0.122500);

    // vec3 normal_tex = texture(s_water, p_uv).rgb * 2.0 - 1.0;
    // vec3 distorted_normal = normalize(mix(p_normal, normal_tex, 0.5));

    // vec3 N = distorted_normal;
    vec3 N = normalize(p_normal);
    vec3 V = normalize(camera_pos - p_world_pos);
    vec3 L = sun_light.direction;
    vec3 H = normalize(V + L);

    float roughness = 0.05;
    float metallic = 0;
    float F0 = 0.02;

    float shadow = ShadowCalculation(p_shadow_pos, N, s_shadow, s_noise);
    vec3 lighting = CalculatePBR(N, V, L, H, roughness, metallic, F0, albedo, shadow);
    float fresnel = mix(0.3, 1.2, FresnelSchlick(max(dot(N, V), 0.0), F0));

    color = vec4(lighting, min(fresnel, 1.0));
}
