struct Globals {
    float4x4 proj_matrix;
    float4x4 view_matrix;
};

struct InstanceData {
    float px, py, pz;
    uint side;
    uint texture;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv;
    uint texture;
};

[[vk::binding(0)]]
ConstantBuffer<Globals> globals;

[[vk::binding(1)]]
StructuredBuffer<InstanceData> instances;

[[vk::binding(2)]]
Texture2D textures[2];

[[vk::binding(2)]]
SamplerState samplers[2];

static const float3 positions[6][4] = {
    { // top
        float3(0.0, 1.0, 0.0),
        float3(0.0, 1.0, 1.0),
        float3(1.0, 1.0, 1.0),
        float3(1.0, 1.0, 0.0)
    },
    { // bot
        float3(0.0, 0.0, 1.0),
        float3(0.0, 0.0, 0.0),
        float3(1.0, 0.0, 0.0),
        float3(1.0, 0.0, 1.0)
    },
    { // west
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 0.0, 1.0),
        float3(0.0, 1.0, 1.0)
    },
    { // east
        float3(1.0, 1.0, 1.0),
        float3(1.0, 0.0, 1.0),
        float3(1.0, 0.0, 0.0),
        float3(1.0, 1.0, 0.0)
    },
    { // north
        float3(0.0, 1.0, 1.0),
        float3(0.0, 0.0, 1.0),
        float3(1.0, 0.0, 1.0),
        float3(1.0, 1.0, 1.0)
    },
    { // south
        float3(1.0, 1.0, 0.0),
        float3(1.0, 0.0, 0.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0)
    }
};

static const float3 normals[6] = {
    float3(0.0, 1.0, 0.0),
    float3(0.0, -1.0, 0.0),
    float3(-1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 0.0, -1.0)
};

static const float2 uvs[4] = {
    float2(0.0, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 1.0),
    float2(1.0, 0.0)
};

static const uint indices[] = {
    0, 1, 2,
    0, 2, 3
};

[shader("vertex")]
VSOutput VSMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
    VSOutput result;

    InstanceData instance = instances[instanceID];
    uint index = indices[vertexID];
    float3 vert_pos = positions[instance.side][index];
    float3 normal = normals[instance.side];
    float2 uv = uvs[index];

    float3 instance_pos = float3(instance.px, instance.py, instance.pz);
    float4 pos = float4(vert_pos + instance_pos, 1.0);

    float4 view_pos = mul(globals.view_matrix, pos);
    result.position = mul(globals.proj_matrix, view_pos);
    result.uv = uv;

    result.texture = instance.texture;

    return result;
}

[shader("pixel")]
float4 PSMain(VSOutput input) : SV_Target {
    float4 result = textures[input.texture].Sample(samplers[input.texture], input.uv);

    return result;
}
