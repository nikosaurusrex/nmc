struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

struct Globals {
    float4x4 proj_matrix;
    float4x4 view_matrix;
};

struct InstanceData {
    float px, py, pz;
    uint texture;
    uint texture_top;
    uint texture_bot;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv;
    uint texture;
};

[[vk::binding(0)]]
StructuredBuffer<Vertex> vertices;

[[vk::binding(1)]]
ConstantBuffer<Globals> globals;

[[vk::binding(2)]]
StructuredBuffer<InstanceData> instances;

[[vk::binding(3)]]
Texture2D textures[2];

[[vk::binding(3)]]
SamplerState samplers[2];

[shader("vertex")]
VSOutput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
    VSOutput result;

    InstanceData instance = instances[instanceID];
    Vertex v = vertices[vertexID];
    float4 pos = float4(v.px + instance.px, v.py + instance.py, v.pz + instance.pz, 1.0);
    float3 normal = float3(v.nx, v.ny, v.nz);

    float4 view_pos = mul(globals.view_matrix, pos);
    result.position = mul(globals.proj_matrix, view_pos);
    result.uv = float2(v.u, v.v);

    uint texture_index;
    if (normal.y > 0.5) {
        texture_index = instance.texture_top;
    } else if (normal.y < -0.5) {
        texture_index = instance.texture_bot;
    } else {
        texture_index = instance.texture;
    }
    result.texture = texture_index;

    return result;
}

[shader("pixel")]
float4 PSMain(VSOutput input) : SV_Target {
    float4 result = textures[input.texture].Sample(samplers[input.texture], input.uv);

    return result;
}
