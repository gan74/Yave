#version 450

#include "../lib/interpolants.glsl"

layout(location = 0) out uint out_id;

DECLARE_STANDARD_INTERPOLANTS(in)

layout(set = 0, binding = 1) readonly buffer Ids {
    uint ids[];
};

void main() {
    out_id = ids[in_instance_index] + 1;
}
