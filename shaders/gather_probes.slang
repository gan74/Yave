#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler3D<uint> in_grid;

[[vk::binding(1)]]
RWStructuredBuffer<uint> out_count;

[[vk::binding(2)]]
cbuffer Params {
    uint frame_id;
};


[shader("compute")]
[numthreads(4, 4, 4)]
void comp() {
    const uint3 size = image_size(in_grid);
    const uint3 coord = semantics.global_id;

    if(any(coord >= size)) {
        return;
    }

    if(in_grid[coord] == frame_id) {
        uint prev;
        InterlockedAdd(out_count[0], 1, prev);
    }
}


