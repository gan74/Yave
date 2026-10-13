#include "lib/structs.slang"
#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/bilateral.slang"


[[vk::binding(0)]]
Sampler2D<float4> in_color;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
RWTexture2D<float4> out_image;

[[vk::binding(5)]]
cbuffer Params {
    float spatial_sigma;
    float depth_phi;
    float normal_phi;
    float padding_0;
};




[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(in_depth);

    if(any(coord >= size)) {
        return;
    }

    const float depth = in_depth[coord];

    if(is_depth_far(depth)) {
        return;
    }

    const float linear_depth = linearize_depth(depth, camera.inv_proj);
    const float3 normal = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[coord])).normal;

    const BilateralUpsampleParams params = { spatial_sigma, depth_phi, normal_phi };
    out_image[coord] = bilateral_upsample(in_color, in_depth, in_rt1, camera.inv_proj, coord, linear_depth, normal, params);
}
