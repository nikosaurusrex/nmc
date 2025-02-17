#define PI 3.14159265359

#define GAMMA 2.2

struct DirectionalLight {
	vec3 color;
	vec3 direction;
};

struct Material {
    float roughness;
    float metallic;
    float F0;
};

const DirectionalLight sun_light = DirectionalLight(
	vec3(1.0, 1.0, 0.9) * 2,
	-normalize(vec3(0.5, -1, 0.5))
);

const vec3 sky_color_1 = vec3(0.478, 0.7, 1.9);
const vec3 sky_color_2 = vec3(0.58, 0.9, 2.1);
const vec3 ambient_light = vec3(0.1, 0.1, 0.15);

const vec2 shadow_offsets[9] = {
    vec2(-0.75, -0.75), vec2(-0.75, 0.0), vec2(-0.75, 0.75),
    vec2(0.0, -0.75), vec2(0.0, 0.0), vec2(0.0, 0.75),
    vec2(0.75, -0.75), vec2(0.75, 0.0), vec2(0.75, 0.75)
};

vec3 SampleLinear(sampler2D tex, vec2 uv) {
    vec4 sampled = texture(tex, uv);
    return pow(sampled.rgb, vec3(GAMMA));
}

vec2 DistortShadow(vec2 position){
    float cd = length(position);
    float factor = mix(1.0f, cd, 0.9f);
    return position / factor;
}

vec3 CalculateDiffuse(vec3 normal, vec3 color) {
	float NdotL = dot(normal, sun_light.direction);
	float diff = max(NdotL, 0.0);

    vec3 diffuse = diff * sun_light.color * color;

    return diffuse;
}

vec3 CalculateSpecular(vec3 normal, vec3 view_dir, float shininess) {
    vec3 hv = normalize(sun_light.direction + view_dir);
    float spec = pow(max(dot(normal, hv), 0.0), shininess);
    return spec * sun_light.color;
}

float ShadowCalculation(vec4 shadow_pos, vec3 normal, sampler2D shadow_map, sampler2D noise_tex) {
    vec3 shadow_coords = shadow_pos.xyz / shadow_pos.w;
    shadow_coords.xy = DistortShadow(shadow_coords.xy);
    shadow_coords.xy = shadow_coords.xy * 0.5 + 0.5;

    if (shadow_coords.z >= 1.0 || shadow_coords.z < 0.0) {
        return 0.0;
    }

    float angle = texture(noise_tex, shadow_coords.xy * 20.0).r * 100.0;
    float cost = cos(angle);
    float sint = cos(angle);
    mat2 rot = mat2(cost, -sint, sint, cost) / textureSize(shadow_map, 0).x;

    // vec2 texel_size = vec2(1.0) / textureSize(shadow_map, 0);
    float shadow = 0.0;
    float bias = max(0.001 * (1.0 - dot(normal, sun_light.direction)), 0.0001);
    for (int i = 0; i < 9; i++) {
        // vec2 shadow_offset = shadow_offsets[i] * texel_size;
        vec2 shadow_offset = rot * shadow_offsets[i];
        float depth = texture(shadow_map, shadow_coords.xy + shadow_offset).r;
        shadow += (shadow_coords.z - bias > depth) ? 1.0 : 0.0;
    }

    return shadow / 9.0;
}

float FresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 CalculatePBR(vec3 N, vec3 V, vec3 L, vec3 H, float roughness, float metallic, float F0, vec3 albedo, float shadow) {
    vec3 vF0 = mix(vec3(F0), albedo, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), vF0);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    vec3 kD = vec3(1.0) - F;
    kD *= 1.0 - metallic;

    vec3 specular = vec3((NDF * G * F) / (4.0 * NdotL * NdotV + 0.001));
    vec3 diffuse = (albedo * kD) / PI;

    vec3 radiance = sun_light.color * NdotL;
    vec3 diff_spec = (diffuse + specular) * radiance;

    vec3 ambient = ambient_light * albedo;

    return ambient + (1.0 - shadow) * diff_spec;
}
