#version 450

struct InstanceData {
    float px, py, pz;
    uint side;
    uint texture;
};

layout(set=0, binding=0) uniform LightSpaceBuffer {
    mat4 light_space_matrix;
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

    gl_Position = light_space_matrix * pos;
}
