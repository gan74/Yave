#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
RWTexture2D<float2> out_min_max;


groupshared uint shared_min;
groupshared uint shared_max;


[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    if(semantics.local_index == 0) {
        shared_min = 0xFFFFFFFF;
        shared_max = 0x00000000;
    }

    GroupMemoryBarrierWithGroupSync();

    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(in_depth);

    const float depth = in_depth[coord];
    const uint udepth = asuint(depth);

    if(coord.x < size.x && coord.y < size.y) {
        InterlockedMin(shared_min, udepth);
        InterlockedMax(shared_max, udepth);
    }

    GroupMemoryBarrierWithGroupSync();

    if(semantics.local_index == 0) {
        out_min_max[semantics.work_group_id.xy] = float2(asfloat(shared_min), asfloat(shared_max));
    }

}

