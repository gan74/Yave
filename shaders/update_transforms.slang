import "glsl";

#include "lib/utils.slang"

[[vk::binding(0)]]
RWStructuredBuffer<TransformableData> out_transformables;

[[vk::binding(1)]]
StructuredBuffer<float4x4> in_transforms;

[[vk::binding(2)]]
StructuredBuffer<uint> in_indices;

[[vk::binding(3)]]
cbuffer Params_Inline {
    uint transform_count;
};


[shader("compute")]
[numthreads(64)]
void comp_main() {
    const uint id = uint(semantics.global_id.x);
    if(id >= transform_count) {
        return;
    }

    const uint packed_index = in_indices[id];
    const bool set_last = (packed_index & 0x80000000) != 0;
    const uint index = packed_index & ~0x80000000;

    out_transformables[index].last = set_last ? in_transforms[id] : out_transformables[index].current;
    out_transformables[index].current = in_transforms[id];
}

