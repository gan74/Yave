#include "../lib/utils.slang"

struct PickingOutput {
    float depth;
    uint entity_id;
};


[[vk::binding(0, 0)]]
Sampler2D<float> in_depth;

[[vk::binding(1, 0)]]
Sampler2D<uint> in_id;

[[vk::binding(2, 0)]]
RWStructuredBuffer<PickingOutput> out;

[[vk::binding(0, 1)]]
cbuffer Data_Inline {
    float2 uv;
};


[shader("compute")]
[numthreads(1, 1)]
void comp_main() {
    const PickingOutput pick = {
        in_depth.SampleLevel(uv, 0),
        in_id[int2(image_size(in_id) * uv)] - 1,
    };

    out[0] = pick;
}

