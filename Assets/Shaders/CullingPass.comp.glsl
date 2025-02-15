#version 450

struct IndirectDrawCommand {
    uint index_count;
    uint instance_count;
    uint first_index;
    int vertex_offset;
    uint first_instance;
};

layout(set=0, binding=0) readonly buffer VisibleCountBuffer {
    uint visible_count;
};

layout(set=0, binding=1) writeonly buffer DrawCommandBuffer {
    IndirectDrawCommand draw_command;
};

layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

void main() {
    draw_command.instance_count = visible_count;
}
