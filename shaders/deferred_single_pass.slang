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
StructuredBuffer<ShadowMapInfo> shadow_infos;

[[vk::binding(6)]]
ConstantBuffer<LightClusterInfo> cluster_info;

[[vk::binding(7)]]
StructuredBuffer<LightTile> light_tiles;

[[vk::binding(8)]]
RWTexture2D out_color;


// -------------------------------- MAIN --------------------------------

[shader("compute")]
[numthreads(light_cluster_tile_size, light_cluster_tile_size)]
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

