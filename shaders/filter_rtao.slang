#include "lib/utils.slang"

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
Sampler2D<float> in_ao;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
cbuffer Weights_Inline {
    float4 weights;
};

[[vk::binding(5)]]
RWTexture2D<float> out_ao;







int2 make_offset(int i) {
#if defined(HORIZONTAL)
    return int2(i, 0);
#else
    return int2(0, i);
#endif
}

int2 to_sample_coord(int2 c) {
#if defined(HORIZONTAL)
    return c / int2(2, 1);
#else
    return c / int2(1, 2);
#endif
}

int2 from_sample_coord(int2 c) {
#if defined(HORIZONTAL)
    return c * 2;
#else
    return c * int2(1, 2);
#endif

}

int2 to_base_coord(int2 c) {
#if defined(HORIZONTAL)
    return c * int2(1, 2);
#else
    return c;
#endif
}





float gaussian_weight(int i) {
    const uint index = abs(i);
    return weights[index];
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
    const int2 coord = int2(semantics.global_id.xy);
    const float2 image_size = float2(image_size(out_ao).xy);

    const float depth = in_depth[to_base_coord(coord)].x;
    const float3 normal = decode_gbuffer({float4(0.0), in_rt1[to_base_coord(coord)]}).normal;

    const float3 world_pos = unproject(coord / image_size, depth, camera.inv_view_proj);
    const float3 view_dir = normalize(camera.position - world_pos);

    const float dot_view_normal = dot(view_dir, normal);

    float ao = 0.0;
    float weight = 0.0;
    for (int i = -3; i <= 3; ++i) {
        const int2 sample_coord = to_sample_coord(coord) + make_offset(i);

        const float sample_depth = in_depth[from_sample_coord(sample_coord)];
        const float3 sample_normal = decode_gbuffer({float4(0.0), in_rt1[from_sample_coord(sample_coord)]}).normal;
        const float sample = in_ao[sample_coord];

        const float w = gaussian_weight(i);

        if(i == 0 || is_valid(sample_depth, depth, sample_normal, normal, dot_view_normal)) {
            ao += sample * w;
            weight += w;
        }
    }

    out_ao[semantics.global_id.xy] = ao / weight;
}
