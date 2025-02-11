struct InstanceData {
    float px, py, pz;
    uint texture;
    uint texture_top;
    uint texture_bot;
};

struct FrustumInfo {
    float4 planes[6];
    uint instance_count;
};

[[vk::binding(0)]]
StructuredBuffer<InstanceData> instances;

[[vk::binding(1)]]
RWStructuredBuffer<InstanceData> culled_instances;

[[vk::binding(2)]]
RWStructuredBuffer<Atomic<uint>> visible_count;

[[vk::binding(3)]]
ConstantBuffer<FrustumInfo> frustum_info;

static const float sphere_radius = 0.8660254;

bool IsInsideFrustum(float4 center) {
    if (dot(frustum_info.planes[0], center) < -sphere_radius) return false;

    return true;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID) {
    uint idx = dispatchThreadID.x;
    if (idx >= frustum_info.instance_count) {
        return;
    }

    InstanceData id = instances[idx];
    float4 center = float4(float3(id.px, id.py, id.pz) + float3(0.5), 1.0);

    if (IsInsideFrustum(center)) {
        culled_instances[idx] = instances[idx];
        visible_count[0].add(1);
    }
}
