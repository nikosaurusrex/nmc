#version 450

struct InstanceData {
    float px, py, pz;
    uint side;
    uint texture;
};

layout(set=0, binding=0) uniform GlobalsUniform {
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 light_space_matrix;
    vec3 camera_pos;
};

layout(set=0, binding=1) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

const vec3 positions[6][4] = {
    { // top
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 1.0, 1.0),
        vec3(1.0, 1.0, 1.0),
        vec3(1.0, 1.0, 0.0)
    },
    { // bot
        vec3(0.0, 0.0, 1.0),
        vec3(0.0, 0.0, 0.0),
        vec3(1.0, 0.0, 0.0),
        vec3(1.0, 0.0, 1.0)
    },
    { // west
        vec3(0.0, 1.0, 0.0),
        vec3(0.0, 0.0, 0.0),
        vec3(0.0, 0.0, 1.0),
        vec3(0.0, 1.0, 1.0)
    },
    { // east
        vec3(1.0, 1.0, 1.0),
        vec3(1.0, 0.0, 1.0),
        vec3(1.0, 0.0, 0.0),
        vec3(1.0, 1.0, 0.0)
    },
    { // north
        vec3(0.0, 1.0, 1.0),
        vec3(0.0, 0.0, 1.0),
        vec3(1.0, 0.0, 1.0),
        vec3(1.0, 1.0, 1.0)
    },
    { // south
        vec3(1.0, 1.0, 0.0),
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0)
    }
};

const vec3 normals[6] = {
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, -1.0, 0.0),
    vec3(-1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.0, -1.0)
};

const vec2 uvs[4] = {
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
};

const uint indices[] = {
    0, 1, 2,
    0, 2, 3
};

layout(location=0) out vec3 p_world_pos;
layout(location=1) out vec3 p_normal;
layout(location=2) out vec2 p_uv;
layout(location=3) out vec4 p_shadow_pos;
layout(location=4) flat out uint p_texture;

void main() {
    uint vertexID = gl_VertexIndex;
    uint instanceID = gl_InstanceIndex;

    InstanceData instance = instances[instanceID];
    uint index = indices[vertexID];
    vec3 vert_pos = positions[instance.side][index];
    vec3 normal = normals[instance.side];
    vec2 uv = uvs[index];

    vec3 instance_pos = vec3(instance.px, instance.py, instance.pz);
    vec4 pos = vec4(vert_pos + instance_pos, 1.0);

    vec4 view_pos = view_matrix * pos;
    gl_Position = proj_matrix * view_pos;

    p_world_pos = pos.xyz;
    p_normal = normal;
    p_uv = uv;
    p_shadow_pos = light_space_matrix * pos;
    p_texture = instance.texture;
}
