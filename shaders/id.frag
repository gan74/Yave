#version 450

#include "lib/interpolants.glsl"

layout(location = 0) out uint out_id;

DECLARE_STANDARD_INTERPOLANTS(in)

layout(set = 1, binding = 2) readonly buffer Indices {
    uvec2 mesh_indices[];
};

void main() {
    out_id = mesh_indices[in_instance_index].y + 1;
}
