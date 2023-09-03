#version 450

layout(location = 0) out uint out_id;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_tangent;
layout(location = 2) in vec3 in_bitangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in vec2 in_motion;
layout(location = 5) in flat uint in_instance_index;

layout(set = 0, binding = 1) readonly buffer Ids {
    uint ids[];
};

void main() {
    out_id = ids[in_instance_index] + 1;
}
