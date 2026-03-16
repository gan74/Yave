

#include "lib/utils.slang"
#include "lib/brdf.slang"
#include "lib/debug.slang"


// -------------------------------- DEFINES --------------------------------

// #define DEBUG


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
ConstantBuffer<Camera> camera;

[[vk::binding(5)]]
StructuredBuffer<DirectionalLight> directional_lights;

[[vk::binding(6)]]
StructuredBuffer<PointLight> point_lights;

[[vk::binding(7)]]
StructuredBuffer<SpotLight> spot_lights;

[[vk::binding(8)]]
StructuredBuffer<ShadowMapInfo> shadow_infos;

[[vk::binding(9)]]
ConstantBuffer<LightClusterInfo> cluster_info;

[[vk::binding(10)]]
StructuredBuffer<LightTile> light_tiles;

[[vk::binding(11)]]
RWTexture2D out_color;



// -------------------------------- HELPERS --------------------------------

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
    if(cluster_info.directional_count > 0) {
        const uint4 indices = directional_lights[0].shadow_map_indices;
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

[shader("compute")]
[numthreads(16, 16)]
void comp_main() {
    const uint2 coord = semantics.global_id.xy;
    const float2 image_size = float2(image_size(out_color).xy);
    const float2 uv = (semantics.global_id.xy + 0.5) / image_size;

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        return;
    }


    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const float3 view_dir = normalize(camera.position - world_pos);

    const SurfaceInfo surface = decode_gbuffer(RawGBuffer(
        in_rt0[coord],
        in_rt1[coord],
    ));

    float3 irradiance = float3(0.0);

    // -------------------------------- DIRECTIONALS --------------------------------
    for(uint i = 0; i != cluster_info.directional_count; ++i) {
        const DirectionalLight light = directional_lights[i];
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

    // -------------------------------- POINTS --------------------------------
    const uint point_begin = 0;
    const uint tile_point_count = light_tiles[semantics.work_group_index].point_count;
    const uint point_end = min(max_light_per_tile, tile_point_count);
    for(uint i = point_begin; i < point_end; ++i) {
        const uint index = light_tiles[semantics.work_group_index].indices[i];
        const PointLight light = point_lights[index];
        const AreaLightInfo area = karis_area_light(surface, light, world_pos, view_dir);
        const float att = attenuation(area.orig_light_dist * light.falloff, light.range * light.falloff, light.min_radius * light.falloff);

        if(att > 0.0) {
            const float3 radiance = light.color * att;
            irradiance += radiance * eval_brdf(alpha_corrected_surface(surface, area), view_dir, area.light_dir);
        }
    }

    // -------------------------------- SPOTS --------------------------------
    const uint spot_begin = tile_point_count;
    const uint tile_spot_count = light_tiles[semantics.work_group_index].spot_count;
    const uint spot_end = min(max_light_per_tile, spot_begin + tile_spot_count);
    for(uint i = spot_begin; i < spot_end; ++i) {
        const uint index = light_tiles[semantics.work_group_index].indices[i];
        const SpotLight light = spot_lights[index];
        const AreaLightInfo area = karis_area_light(surface, light, world_pos, view_dir);
        float att = attenuation(area.orig_light_dist * light.falloff, light.range * light.falloff, light.min_radius * light.falloff);

        if(att > 0.0) {
            const float spot_cos_alpha = max(0.0, -dot(area.light_dir, light.forward));
            att *= spot_attenuation(spot_cos_alpha, light.att_scale_offset);
        }

        if(att > 0.0 && light.shadow_map_index < 0xFFFFFFFF) {
            const ShadowMapInfo info = shadow_infos[light.shadow_map_index];
            att *= compute_shadow_pcf(in_shadows, info, world_pos);
        }

        if(att > 0.0) {
            const float3 radiance = light.color * att;
            irradiance += radiance * eval_brdf(alpha_corrected_surface(surface, area), view_dir, area.light_dir);
        }
    }

#ifdef DEBUG
    {
        const float total_lights = float(tile_point_count + tile_spot_count);
        float3 heat = heat_spectrum(total_lights / 16.0f);
        heat = lerp(heat, float3(1.0) - heat, debug.print_value(semantics.local_id.xy * 2.0, float2(0.0), float2(8.0, 15.0), total_lights, 2.0, 0.0));
        irradiance = lerp(heat, irradiance, 0.8);
    }
#endif

    out_color[coord] += float4(irradiance, 1.0);
}

