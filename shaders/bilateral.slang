#include "lib/semantics.slang"
#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
RWTexture2D out_image;

[[vk::binding(4)]]
cbuffer Weights {
    float weights[32];
};

[[vk::binding(5)]]
cbuffer Params {
    float depth_phi;
    float normal_phi;
};




#ifdef HORIZONTAL
static const int2 direction = int2(1, 0);
#else
static const int2 direction = int2(0, 1);
#endif

static const uint kernel_size = 32;

#define DEPTH_WEIGHT
#define NORMAL_WEIGHT

void half_kernel(inout float4 acc, inout float total, int2 dir, float center_depth, float3 center_normal) {
    for(uint i = 1; i < kernel_size; ++i) {
        const uint2 coord = semantics.global_id.xy + dir * i;

        float weight = weights[i];

#ifdef DEPTH_WEIGHT
        {
            const float depth = in_depth[coord];
            const float delta = abs(center_depth - depth);
            weight *= min(exp(-delta / depth_phi), 1.0);
        }
#endif

#ifdef NORMAL_WEIGHT
        {
            const float3 normal = decode_gbuffer({float4(0.0), in_rt1[coord]}).normal;
            const float delta = saturate(dot(center_normal, normal));
            weight *= min(pow(delta, normal_phi), 1.0);
        }
#endif

        acc += in_color[coord + dir * i] * weight;
        total += weight;
    }
}

[shader("compute")]
[numthreads(32, 1)]
void comp() {
    const uint2 size = image_size(in_depth);
    const uint2 coord = semantics.global_id.xy;

    if (!all(coord < size)) {
        return;
    }

    float total = weights[0];
    float4 acc = in_color[coord] * total;

    const float depth = in_depth[coord];
    const float3 normal = decode_gbuffer({float4(0.0), in_rt1[coord]}).normal;

    half_kernel(acc, total, direction, depth, normal);
    half_kernel(acc, total, -direction, depth, normal);

    out_image[coord] = acc / total;
}
