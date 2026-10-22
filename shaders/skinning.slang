#include "lib/utils.slang"

[[vk::binding(0)]]
RWStructuredBuffer<float> out_positions;

[[vk::binding(1)]]
StructuredBuffer<float> in_positions;

[[vk::binding(2)]]
StructuredBuffer<uint4> skin_weights;

[[vk::binding(3)]]
StructuredBuffer<float4x4> bone_transforms;

[[vk::binding(4)]]
cbuffer Params {
    uint vertex_count;
};


[shader("compute")]
[numthreads(64)]
void comp_main() {
    const uint id = semantics.global_id.x;
    if(id >= vertex_count) {
        return;
    }

    const uint4 skin = skin_weights[id];
    const uint4 indices = uint4(skin.x & 0xFFFF, skin.x >> 16, skin.y & 0xFFFF, skin.y >> 16);
    const float4 weights = float4(skin.z & 0xFFFF, skin.z >> 16, skin.w & 0xFFFF, skin.w >> 16) / float((1 << 16) - 1);

    const float4x4 bone_transform =
        bone_transforms[indices.x] * weights.x +
        bone_transforms[indices.y] * weights.y +
        bone_transforms[indices.z] * weights.z +
        bone_transforms[indices.w] * weights.w;

    const float3 in_position = float3(in_positions[id * 3 + 0], in_positions[id * 3 + 1], in_positions[id * 3 + 2]);
    const float3 position = mul(bone_transform, float4(in_position, 1.0)).xyz;

    out_positions[id * 3 + 0] = position.x;
    out_positions[id * 3 + 1] = position.y;
    out_positions[id * 3 + 2] = position.z;
}

