import "glsl";

#include "../lib/utils.slang"

[[vk::binding(0)]]
cbuffer Target_Inline {
    uint target_index;
};


[[vk::binding(1)]]
Sampler2D in_final;

[[vk::binding(2)]]
Sampler2D in_depth;

[[vk::binding(3)]]
Sampler2D in_motion;

[[vk::binding(4)]]
Sampler2D in_rt0;

[[vk::binding(5)]]
Sampler2D in_rt1;

[[vk::binding(6)]]
Sampler2D in_ao;


[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const int2 coord = int2(gl_FragCoord.xy);

    const SurfaceInfo surface = decode_gbuffer({
        in_rt0[coord],
        in_rt1[coord],
    });

    const float3 final = in_final[coord].rgb;

    float3 color = final;
    if(target_index == 1) {
        color = surface.albedo;
    } else if(target_index == 2) {
        color = surface.normal * 0.5 + 0.5;
    } else if(target_index == 3) {
        color = float3(surface.metallic);
    } else if(target_index == 4) {
        color = float3(surface.perceptual_roughness);
    } else if(target_index == 5) {
        const float depth = in_depth[coord].r;
        color = float3(pow(depth, 0.35));
    } else if(target_index == 6) {
        const vec2 motion = in_motion[coord].xy;
        color = float3(clamp(motion * 10.0, -0.5, 0.5) + 0.5, 0.5);
    } else if(target_index == 7) {
        const float ao = in_ao.Sample(in_uv).r;
        color = float3(ao);
    }

    return float4(color, 1.0);
}
