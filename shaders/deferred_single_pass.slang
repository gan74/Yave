#include "lib/lighting.slang"
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
    const SurfaceInfo surface = decode_gbuffer(RawGBuffer(
        in_rt0[coord],
        in_rt1[coord],
    ));

    float3 irradiance = compute_lighting(
        surface,
        world_pos,
        normalize(camera.position - world_pos),
        cluster_info,
        light_tiles[semantics.work_group_index],
        directional_lights,
        point_lights,
        spot_lights,
        shadow_infos,
        in_shadows
    );
#ifdef DEBUG
    {
        const LightTile light_tile = light_tiles[semantics.work_group_index];
        const float total_lights = float(light_tile.spot_count + light_tile.point_count);
        float3 heat = heat_spectrum(total_lights / 16.0f);
        heat = lerp(heat, float3(1.0) - heat, debug.print_value(semantics.local_id.xy * 2.0, float2(0.0), float2(8.0, 15.0), total_lights, 2.0, 0.0));
        irradiance = lerp(heat, irradiance, 0.8);
    }
#endif

    out_color[coord] += float4(irradiance, 1.0);
}

