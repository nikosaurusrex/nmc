struct Vertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

struct Globals {
    float4x4 proj_matrix;
    float4x4 view_matrix;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv;
};

[[vk::binding(0)]]
StructuredBuffer<Vertex> vertices;

[[vk::binding(1)]]
ConstantBuffer<Globals> globals;

[[vk::binding(2)]]
Texture2D texture;

[[vk::binding(2)]]
SamplerState sampler;

[shader("vertex")]
VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput result;

    Vertex v = vertices[vertexID];
    float4 pos = float4(v.px, v.py, v.pz, 1.0);

    float4 view_pos = mul(globals.view_matrix, pos);
    result.position = mul(globals.proj_matrix, view_pos);
    result.uv = float2(v.u, v.v);

    return result;
}

[shader("pixel")]
float4 PSMain(VSOutput input) : SV_Target {
    float4 result = texture.Sample(sampler, input.uv);

    return result;
}
