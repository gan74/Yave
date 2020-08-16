#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec2 in_size;
layout(location = 3) in uint in_id;

layout(location = 0) out uint out_instance_id;
layout(location = 1) out vec3 out_position;
layout(location = 2) out vec2 out_uv;
layout(location = 3) out vec2 out_size;


void main() {
    out_position = in_position;
    out_uv = in_uv;
    out_size = in_size;
    out_instance_id = in_id;
}

