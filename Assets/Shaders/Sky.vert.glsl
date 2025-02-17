#version 450

layout(location=0) out vec2 p_uv;

const vec2 vertices[] = {
    vec2(-1.0, -1.0), vec2(-1.0, 1.0), vec2(1.0, 1.0),
    vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(1.0, -1.0)
};

const vec2 uvs[] = {
    vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0),
    vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 0.0)
};

void main() {
    uint idx = gl_VertexIndex;

    vec4 pos = vec4(vertices[idx], 0.0, 1.0);

    p_uv = uvs[idx];

    gl_Position = pos;
}
