struct IndirectDrawCommand {
    uint index_count;
    uint instance_count;
    uint first_index;
    int vertex_offset;
    uint first_instance;
};

[[vk::binding(0)]]
StructuredBuffer<uint> visible_count;

[[vk::binding(1)]]
RWStructuredBuffer<IndirectDrawCommand> draw_command;

[numthreads(1, 1, 1)]
void CSMain() {
    draw_command[0].instance_count = visible_count[0];
}
