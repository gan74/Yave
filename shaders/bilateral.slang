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





void half_kernel(inout float4 acc, inout float total, int2 dir, float center_depth, float3 center_normal, float2 center_uv, float2 inv_size) {
    for(uint i = 1; i < kernel_size; ++i) {
        const float2 uv = center_uv + inv_size * dir * i;

        float weight = weights[i];

        {
            const float depth = in_depth.SampleLevel(uv, 0.0);
            const float delta = abs(center_depth - depth);
            weight *= min(exp(-delta / depth_phi), 1.0);
        }

        {
            const float3 normal = decode_gbuffer({float4(0.0), in_rt1.SampleLevel(uv, 0.0)}).normal;
            const float delta = saturate(dot(center_normal, normal));
            weight *= min(pow(delta, normal_phi), 1.0);
        }

        acc += in_color.SampleLevel(uv, 0.0) * weight;
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

    const float2 inv_size = 1.0 / float2(image_size(out_image).xy);
    const float2 center_uv = (semantics.global_id.xy + 0.5) * inv_size;

    float total = weights[0];
    float4 acc = in_color.SampleLevel(center_uv, 0.0) * total;

    const float depth = in_depth.SampleLevel(center_uv, 0.0);
    const float3 normal = decode_gbuffer({float4(0.0), in_rt1.SampleLevel(center_uv, 0.0)}).normal;

    half_kernel(acc, total, direction, depth, normal, center_uv, inv_size);
    half_kernel(acc, total, -direction, depth, normal, center_uv, inv_size);

    out_image[coord] = acc / total;
}
