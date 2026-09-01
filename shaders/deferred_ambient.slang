#include "lib/utils.slang"
#include "lib/brdf.slang"

// -------------------------------- DEFINES --------------------------------

#ifdef __cplusplus
#define GI
#define AO
#endif

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2D in_rt0;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
Sampler2D in_gi_or_ao;

[[vk::binding(4)]]
SamplerCube in_envmap;

[[vk::binding(5)]]
Sampler2D<float2> brdf_lut;

[[vk::binding(6)]]
ConstantBuffer<Camera> camera;

[[vk::binding(7)]]
RWTexture2D out_color;

[[vk::binding(8)]]
cbuffer Params {
    uint display_sky;
    float ibl_intensity;
};






float3 view_direction(Camera camera, float2 uv) {
    return normalize(unproject(uv, 0.001, camera.curr.inv_view_proj) - camera.position);
}



// -------------------------------- MAIN --------------------------------
[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const uint2 coord = semantics.global_id.xy;
    const float2 image_size = float2(image_size(out_color).xy);
    const float2 uv = (semantics.global_id.xy + 0.5) / image_size;

    const float depth = in_depth[coord];

    if(is_depth_far(depth)) {
        const float3 view_dir = view_direction(camera, uv);
        out_color[coord] = float4(display_sky != 0 ? in_envmap.SampleLevel(view_dir, 0.0).rgb : float3(0.0), 1.0);
        return;
    }

    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);

    float3 view_dir = (camera.position - world_pos);
    const float view_dist = length(view_dir);
    view_dir /= view_dist;

    const SurfaceInfo surface = decode_gbuffer(RawGBuffer(
        in_rt0[coord],
        in_rt1[coord],
    ));

#ifdef GI
    const float3 irradiance = eval_ibl(RTGIProbe(in_envmap, in_gi_or_ao[coord].rgb), brdf_lut, view_dir, surface) * ibl_intensity;
#endif

#ifdef AO
    const float3 irradiance = eval_ibl(IBLProbe(in_envmap), brdf_lut, view_dir, surface) * ibl_intensity * in_gi_or_ao[coord].r;
#endif

    out_color[coord] += float4(irradiance, 1.0);
}

