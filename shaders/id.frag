#version 450

layout(location = 0) out uint out_id;

layout(location = 0) in flat uint in_instance_id;

layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

void main() {
    out_id = in_instance_id + 1;
}

