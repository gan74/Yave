#include "../lib/utils.slang"

[[vk::binding(0)]]
cbuffer Target_Inline {
    uint target_index;
};


[[vk::binding(1)]]
Sampler2D in_final;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D<float2> in_motion;

[[vk::binding(4)]]
Sampler2D in_rt0;

[[vk::binding(5)]]
Sampler2D in_rt1;

[[vk::binding(6)]]
Sampler2D<uint> in_mask;

[[vk::binding(7)]]
Sampler2D<float> in_ao;


[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const int2 coord = int2(semantics.frag_coord.xy);

    const SurfaceInfo surface = decode_gbuffer({
        in_rt0[coord],
        in_rt1[coord],
    });

    const float3 final = in_final[coord].rgb;

    float3 color = linear_to_sRGB(final);
    if(target_index == 1) {
        color = surface.albedo;
    } else if(target_index == 2) {
        color = surface.normal * 0.5 + 0.5;
    } else if(target_index == 3) {
        color = float3(surface.metallic);
    } else if(target_index == 4) {
        color = float3(surface.perceptual_roughness);
    } else if(target_index == 5) {
        const float depth = in_depth[coord];
        color = float3(pow(depth, 0.35));
    } else if(target_index == 6) {
        const float2 motion = in_motion[coord];
        color = float3(clamp(motion * 10.0, -0.5, 0.5) + 0.5, length(motion) > epsilon ? 0.75 : 0.5);
    } else if(target_index == 7) {
        const float3 colors[] = {
            float3(pow(in_depth[coord], 0.35)),
            float3(1, 0, 0), float3(0, 1, 0), float3(1, 1, 0), float3(1, 0, 1)
        };
        color = colors[in_mask[coord]];
    } else if(target_index == 8) {
        const float ao = in_ao.Sample(in_uv);
        color = float3(ao);
    }

    return float4(color, 1.0);
}

