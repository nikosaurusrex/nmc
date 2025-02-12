struct InstanceData {
    float px, py, pz;
    uint side;
    uint texture;
};

struct FrustumInfo {
    float4x4 view_matrix;
    float4 planes[6];
    uint instance_count;
};

[[vk::binding(0)]]
StructuredBuffer<InstanceData> instances;

[[vk::binding(1)]]
RWStructuredBuffer<InstanceData> culled_instances;

[[vk::binding(2)]]
RWStructuredBuffer<uint> visible_count;

[[vk::binding(3)]]
ConstantBuffer<FrustumInfo> frustum_info;

bool IsInsideFrustum(float4 pos) {
    float4 view_pos = mul(frustum_info.view_matrix, pos);
    const float epsilon = -2;

    for (int i = 0; i < 6; ++i) {
        if (dot(view_pos, frustum_info.planes[i]) < epsilon) {
            return false;
        }
    }

    return true;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint idx = dispatchThreadID.x;
    if (idx >= frustum_info.instance_count) {
        return;
    }

    InstanceData id = instances[idx];

    if (IsInsideFrustum(float4(id.px, id.py, id.pz, 1.0))) {
        uint count;
        InterlockedAdd(visible_count[0], 1, count);
        culled_instances[count] = id;
    }
}
