#include "lib/utils.slang"
#include "lib/lighting.slang"

// -------------------------------- DEFINES --------------------------------

#define USE_IBL
#define USE_AO

// #define DEBUG_CASCADES

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2D in_rt0;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
Sampler2DShadow in_shadows;

[[vk::binding(4)]]
Sampler2D<float> in_ao;

[[vk::binding(5)]]
SamplerCube in_envmap;

[[vk::binding(6)]]
Sampler2D<float2> brdf_lut;

[[vk::binding(7)]]
ConstantBuffer<Camera> camera;

[[vk::binding(8)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(9)]]
StructuredBuffer<ShadowMapInfo> shadow_infos;

[[vk::binding(10)]]
cbuffer Params {
    uint light_count;
    uint display_sky;
    float ibl_intensity;
    float padding_;
};






// -------------------------------- HELPERS --------------------------------

float3 view_direction(Camera camera, float2 uv) {
    return normalize(camera.position - unproject(uv, 0.5, camera.curr.inv_view_proj));
}

uint shadow_cascade_index(uint4 indices, float3 world_pos) {
    for(uint i = 0; i != 4; ++i) {
        const uint index = indices[i];
        if(index >= 0xFFFFFFFF) {
            break;
        }

        const ShadowMapInfo sm_info = shadow_infos[index];
        const float2 coords = abs(project(world_pos, sm_info.view_proj).xy * 2.0 - 1.0);
        if(max(coords.x, coords.y) < 1.0 - (sm_info.texel_size * 4.0)) {
            return index;
        }
    }

    return 0xFFFFFFFF;
}

float3 cascade_debug_color(uint shadow_map_index, uint4 indices) {
    if(shadow_map_index == indices[0]) {
        return float3(0.0, 0.0, 1.0);
    }
    if(shadow_map_index == indices[1]) {
        return float3(0.0, 1.0, 0.0);
    }
    if(shadow_map_index == indices[2]) {
        return float3(1.0, 1.0, 0.0);
    }
    return float3(1.0, 0.0, 0.0);
}

float4 cascade_debug_color(float3 world_pos) {
    if(light_count > 0) {
        const uint4 indices = lights[0].shadow_map_indices;
        const uint shadow_map_index = shadow_cascade_index(indices, world_pos);

        if(shadow_map_index < 0xFFFFFFFF) {
            const ShadowMapInfo sm_info = shadow_infos[shadow_map_index];
            const float3 proj = project(world_pos, sm_info.view_proj);
            const float2 texel = floor(atlas_uv(sm_info, proj.xy) * sm_info.size);
            const float3 color = lerp(uv_debug_color(texel), cascade_debug_color(shadow_map_index, indices), 0.25);
            return float4(color, 1.0);
        }
    }
    return float4(0.0);
}






// -------------------------------- MAIN --------------------------------

[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const int2 coord = int2(semantics.frag_coord.xy);

    const float depth = in_depth[coord];
    float3 irradiance = float3(0.0);

    if(is_depth_far(depth)) {
        const float3 view_dir = -view_direction(camera, in_uv);

#if defined(USE_IBL)
        if(display_sky != 0) {
            irradiance = in_envmap.SampleLevel(view_dir, 0.0).rgb;
        }
#endif

        for(uint i = 0; i != light_count; ++i) {
            const DirectionalLight light = lights[i];
            if(dot(view_dir, light.direction) > light.cos_disk) {
                irradiance = light.color;
            }
        }
    } else {
        const SurfaceInfo surface = decode_gbuffer(RawGBuffer(
            in_rt0[coord],
            in_rt1[coord],
        ));

        const float3 world_pos = unproject(in_uv, depth, camera.curr.inv_view_proj);
        const float3 view_dir = normalize(camera.position - world_pos);

        // directional lights
        for(uint i = 0; i != light_count; ++i) {
            const DirectionalLight light = lights[i];
            const float3 light_dir = light.direction; // assume normalized

            float att = 1.0;

            const uint shadow_map_index = shadow_cascade_index(light.shadow_map_indices, world_pos);
            if(shadow_map_index < 0xFFFFFFFF) {
                const ShadowMapInfo sm_info = shadow_infos[shadow_map_index];
                att = compute_shadow_pcf(in_shadows, sm_info, world_pos);
            }

            if(att > 0.0) {
                const float3 radiance = light.color * att;
                irradiance += radiance * eval_brdf(surface, view_dir, light_dir);
            }
        }

#ifdef USE_IBL
        irradiance += eval_ibl(in_envmap, brdf_lut, view_dir, surface) * ibl_intensity;
#endif

#ifdef USE_AO
        irradiance *= in_ao.Sample(in_uv);
#endif

#if defined(DEBUG_CASCADES)
        const float4 debug_color = cascade_debug_color(world_pos);
        irradiance = lerp(irradiance, debug_color.rgb, debug_color.a);
#endif
    }

    return float4(irradiance, 1.0);
}

