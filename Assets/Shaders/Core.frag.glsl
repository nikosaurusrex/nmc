#version 450

#extension GL_EXT_nonuniform_qualifier: require

struct DirectionalLight {
	vec3 color;
	vec3 direction;
};

layout(set=0, binding=0) uniform GlobalsUniform {
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 light_space_matrix;
    vec3 camera_pos;
};

layout(set=0, binding=2) uniform sampler2D s_textures[];
layout(set=0, binding=3) uniform sampler2D s_shadow;

layout(location=0) in vec3 p_world_pos;
layout(location=1) in vec3 p_normal;
layout(location=2) in vec2 p_uv;
layout(location=3) in vec4 p_shadow_pos;
layout(location=4) flat in uint p_texture;

layout(location=0) out vec4 color;

const DirectionalLight dir_light = DirectionalLight(
	vec3(0.85),
	vec3(1, -1, 1)
);

vec3 CalculateDirectionalLight(vec3 normal, vec3 view_dir, vec3 color) {
    vec3 light_dir = normalize(-dir_light.direction);

	float NdotL = dot(normal, light_dir);
	float diff = max(NdotL, 0.0);

    vec3 diffuse = diff * dir_light.color * color;

    /*
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 16.0);
    vec3 specular = spec * dir_light.color;*/

    return diffuse;
}

float ShadowCalculation() {
    vec3 shadow_coords = p_shadow_pos.xyz / p_shadow_pos.w;
    shadow_coords.xy = shadow_coords.xy * 0.5 + 0.5;
    float closest_depth = texture(s_shadow, shadow_coords.xy).r;
    float current_depth = shadow_coords.z;
    float bias = 0.0001;
    float shadow = current_depth - bias > closest_depth ? 1.0 : 0.0;

    if (shadow_coords.z > 1.0) {
        shadow = 0.0;
    }

    return shadow;
}

void main() {
    vec4 result = texture(s_textures[p_texture], p_uv);

    vec3 view_dir = normalize(camera_pos - p_world_pos);

    vec3 tex = result.rgb;
    vec3 ambient = vec3(0.1);

   	vec3 shade = CalculateDirectionalLight(p_normal, view_dir, tex);
    float shadow = ShadowCalculation();

    vec3 lighting = (ambient + (1.0 - shadow) * shade) * tex;
   // vec3 lighting = (ambient + shade) * tex;

    color = vec4(lighting, 1.0);
}
