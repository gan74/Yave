#include "lib/utils.slang"

// -------------------------------- DEFINES --------------------------------

#define KERNEL_SIZE 4

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
Sampler2D<float> in_ao;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
RWTexture2D<float> out_ao;

[[vk::binding(5)]]
cbuffer Params_Inline {
    float4 weights[KERNEL_SIZE / 4];
    int2 offset;
};





int2 make_offset(int i) {
    return offset * i;
}

float gaussian_weight(int i) {
    const uint index = abs(i);
    return weights[index / 4][index % 4];
}

bool is_valid(float sample_depth, float depth, float3 sample_normal, float3 normal, float dot_view_normal) {
    const float depth_epsilon = lerp(0.02, 0.003, dot_view_normal);

    if(abs(1.0f - (sample_depth / depth)) > depth_epsilon) {
        return false;
    }

    if(dot(sample_normal, normal) < 0.9) {
        return false;
    }

    return true;
}

// based on, https://github.com/boksajak/RTAO/blob/master/shaders/RTAOLowPassFilter.hlsl
[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy;
    const float2 image_size = float2(image_size(in_depth).xy);

    const float depth = in_depth[coord].x;
    const float3 normal = decode_gbuffer({float4(0.0), in_rt1[coord]}).normal;

    const float3 world_pos = unproject((coord + 0.5) / image_size, depth, camera.curr.inv_view_proj);
    const float3 view_dir = normalize(camera.position - world_pos);

    const float dot_view_normal = dot(view_dir, normal);

    float ao = 0.0;
    float weight = 0.0;
    for (int i = -(KERNEL_SIZE - 1); i <= (KERNEL_SIZE - 1); ++i) {
        const int2 sample_coord = coord + make_offset(i);

        const float sample_depth = in_depth[sample_coord];
        const float3 sample_normal = decode_gbuffer({float4(0.0), in_rt1[sample_coord]}).normal;

        const float w = gaussian_weight(i);

        if(is_valid(sample_depth, depth, sample_normal, normal, dot_view_normal)) {
            const float sample = in_ao[sample_coord];
            ao += sample * w;
            weight += w;
        }
    }

    if(weight > 0.0) {
        ao /= weight;
    }

    out_ao[coord] = ao;
}
