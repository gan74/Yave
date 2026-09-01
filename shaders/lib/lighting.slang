#include "utils.slang"
#include "brdf.slang"


uint shadow_cascade_index(StructuredBuffer<ShadowMapInfo> shadow_infos, uint4 indices, float3 world_pos) {
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

float4 cascade_debug_color(float3 world_pos, uint index, LightClusterInfo cluster_info, StructuredBuffer<ShadowMapInfo> shadow_infos) {
    if(cluster_info.directional_count > index) {
        const uint4 indices = cluster_info.directional_lights[index].shadow_map_indices;
        const uint shadow_map_index = shadow_cascade_index(shadow_infos, indices, world_pos);

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




float3 compute_lighting(SurfaceInfo surface,
                        float3 world_pos,
                        float3 view_dir,
                        LightClusterInfo cluster_info,
                        LightTile tile,
                        StructuredBuffer<ShadowMapInfo> shadow_infos,
                        Sampler2DShadow in_shadows) {

    float3 irradiance = float3(0.0);

    // -------------------------------- DIRECTIONALS --------------------------------
    for(uint i = 0; i != cluster_info.directional_count; ++i) {
        const DirectionalLight light = cluster_info.directional_lights[i];
        const float3 light_dir = light.direction; // assume normalized

        float att = 1.0;
        const uint shadow_map_index = shadow_cascade_index(shadow_infos, light.shadow_map_indices, world_pos);
        if(shadow_map_index < 0xFFFFFFFF) {
            const ShadowMapInfo sm_info = shadow_infos[shadow_map_index];
            att = compute_shadow_pcf(in_shadows, sm_info, world_pos);
        }

        if(att > 0.0) {
            const float3 radiance = light.color * att;
            irradiance += radiance * eval_brdf(surface, view_dir, light_dir);
        }
    }

    /*if(tile.point_count + tile.spot_count > max_light_per_tile) {
        return float3(1, 0, 1);
    }*/

    // -------------------------------- POINTS --------------------------------
    const uint point_begin = 0;
    const uint tile_point_count = tile.point_count;
    const uint point_end = min(max_light_per_tile, tile_point_count);
    for(uint i = point_begin; i < point_end; ++i) {
        const uint index = tile.indices[i];
        const PointLight light = cluster_info.point_lights[index];
        const AreaLightInfo area = karis_area_light(surface, light, world_pos, view_dir);
        const float att = attenuation(area.orig_light_dist * light.falloff, light.range * light.falloff, light.min_radius * light.falloff);

        if(att > 0.0) {
            const float3 radiance = light.color * att;
            irradiance += radiance * eval_brdf(alpha_corrected_surface(surface, area), view_dir, area.light_dir);
        }
    }

    // -------------------------------- SPOTS --------------------------------
    const uint spot_begin = tile_point_count;
    const uint tile_spot_count = tile.spot_count;
    const uint spot_end = min(max_light_per_tile, spot_begin + tile_spot_count);
    for(uint i = spot_begin; i < spot_end; ++i) {
        const uint index = tile.indices[i];
        const SpotLight light = cluster_info.spot_lights[index];
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

    return irradiance;
}
