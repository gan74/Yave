#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2D<uint> in_id;

[[vk::binding(2)]]
RWStructuredBuffer<PickingData> out;

[[vk::binding(3)]]
cbuffer Params {
    uint2 coord;
};


[shader("compute")]
[numthreads(1, 1)]
void comp_main() {
    const PickingData pick = {
        in_depth[coord],
        in_id[coord] - 1,
    };

    out[0] = pick;
}

