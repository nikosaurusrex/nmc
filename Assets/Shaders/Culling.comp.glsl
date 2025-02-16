#version 450

struct InstanceData {
    float px, py, pz;
    uint side;
    uint texture;
};

struct FrustumInfo {
    mat4 view_matrix;
    vec4 planes[6];
    uint instance_count;
};

struct IndirectDrawCommand {
    uint index_count;
    uint instance_count;
    uint first_index;
    int vertex_offset;
    uint first_instance;
};

layout(set=0, binding=0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

layout(set=0, binding=1) writeonly buffer CulledInstanceBuffer {
    InstanceData culled_instances[];
};

layout(set=0, binding=2) writeonly buffer DrawCommandBuffer {
    IndirectDrawCommand draw_command;
};

layout(set=0, binding=3) uniform FrustumInfoBuffer {
    FrustumInfo frustum_info;
};

bool IsInsideFrustum(vec4 pos) {
    vec4 view_pos = frustum_info.view_matrix * pos;
    const float epsilon = -2;

    for (int i = 0; i < 6; ++i) {
        if (dot(view_pos, frustum_info.planes[i]) < epsilon) {
            return false;
        }
    }

    return true;
}

layout(local_size_x=64, local_size_y=1, local_size_z=1) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= frustum_info.instance_count) {
        return;
    }

    InstanceData id = instances[idx];

    if (IsInsideFrustum(vec4(id.px, id.py, id.pz, 1.0))) {
        uint count = atomicAdd(draw_command.instance_count, 1);
        culled_instances[count] = id;
    }
}
