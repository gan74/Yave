#include "lib/utils.slang"
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
Sampler2D in_rt1;

[[vk::binding(3)]]
Sampler2DShadow in_shadows;

[[vk::binding(4)]]
ConstantBuffer<Camera> camera;

[[vk::binding(5)]]
StructuredBuffer<PointLight> point_lights;

[[vk::binding(6)]]
StructuredBuffer<SpotLight> spot_lights;

[[vk::binding(7)]]
StructuredBuffer<ShadowMapInfo> shadow_infos;

[[vk::binding(8)]]
RWTexture2D out_color;

[[vk::binding(0, 1)]]
cbuffer LightCount_Inline {
    uint point_count;
    uint spot_count;
};


// -------------------------------- SHARED --------------------------------

groupshared uint light_indices[max_tile_lights];
groupshared uint tile_point_count;
groupshared uint tile_spot_count;

groupshared float3 tile_dir;
groupshared float tile_sin_alpha;

groupshared uint2 tile_dist_bounds;


// -------------------------------- HELPERS --------------------------------

uint2 dispatch_range(uint num) {
    const uint thread_count = semantics.work_group_size.x * semantics.work_group_size.y;
    const uint range = 1 + (num / thread_count);
    const uint begin = min(semantics.local_index * range, num);
    const uint end = min(begin + range, num);
    return uint2(begin, end);
}


// -------------------------------- TILE INFOS --------------------------------

void compute_tile_culling_info(float2 image_size) {
    const float2 corner_uv = (semantics.work_group_id.xy * semantics.work_group_size.xy) / image_size;
    const float3 corner_unproj = unproject_ndc(float3(corner_uv * 2.0 - 1.0, 1.0), camera.cur.inv_view_proj);
    const float3 corner_dir = normalize(corner_unproj - camera.position);


    const float2 center_uv = ((semantics.work_group_id.xy + 0.5) * semantics.work_group_size.xy) / image_size;
    const float3 unproj = unproject_ndc(float3(center_uv * 2.0 - 1.0, 1.0), camera.cur.inv_view_proj);
    tile_dir = normalize(unproj - camera.position);

    const float tile_cos_alpha = dot(tile_dir, corner_dir);
    tile_sin_alpha = sqrt(1.0 - sqr(tile_cos_alpha));
}

void compute_tile_infos(float2 image_size, float view_dist, bool is_oob) {
    if(semantics.local_index == 0) {
        compute_tile_culling_info(image_size);
        tile_point_count = 0;
        tile_spot_count = 0;
        tile_dist_bounds =  uint2(asuint(max_float), 0);
    }

    GroupMemoryBarrierWithGroupSync();

    if(!is_oob) {
        const uint ui_dist = asuint(view_dist);
        InterlockedMin(tile_dist_bounds.x, ui_dist);
        InterlockedMax(tile_dist_bounds.y, ui_dist);
    }

    GroupMemoryBarrierWithGroupSync();
}


// -------------------------------- CULLING --------------------------------

bool is_inside(PointLight light) {
    const float3 to_light = light.position - camera.position;
    const float cos_alpha = max(0.0, dot(to_light, tile_dir));

    if(cos_alpha + light.range < asfloat(tile_dist_bounds.x) ||
       cos_alpha - light.range > asfloat(tile_dist_bounds.y)) {
        return false;
    }

    const float3 closest = camera.position + tile_dir * cos_alpha;
    const float tile_radius = cos_alpha * tile_sin_alpha;
    const float3 to_closest = closest - light.position;
    return dot(to_closest, to_closest) <= sqr(light.range + tile_radius);
}

bool is_inside(SpotLight light) {
    const float cos_alpha = max(0.0, dot(light.encl_sphere_center - camera.position, tile_dir));

    if(cos_alpha + light.encl_sphere_radius < asfloat(tile_dist_bounds.x) ||
       cos_alpha - light.encl_sphere_radius > asfloat(tile_dist_bounds.y)) {
        return false;
    }

    // https://iquilezles.org/articles/intersectors/
    // return iRoundedCone(camera.position, tile_dir, light.position, light.position + light.forward * light.range, tile_radius, light.sin_angle * light.range + tile_radius);

    // Doesn't cull "behind" the cone
    // We still run the bound test on the enclosing sphere, but it's not great

    const float tile_radius = (cos_alpha + light.encl_sphere_radius) * tile_sin_alpha;
    const float3 begin = light.position;
    const float3 end = light.position + light.forward * light.range;
    const float begin_radius = tile_radius;
    const float end_radius = light.sin_angle * light.range + tile_radius;
    const float3 cone_vec = end - begin;
    const float3 to_begin = camera.position - begin;
    const float3 to_end = camera.position - end;
    const float taper = begin_radius - end_radius;

    const float m0 = dot(cone_vec, cone_vec);
    const float m1 = dot(cone_vec, to_begin);
    const float m2 = dot(cone_vec, tile_dir);
    const float m3 = dot(to_begin, tile_dir);
    const float m5 = dot(to_begin, to_begin);
    const float m6 = dot(to_end, tile_dir);
    const float m7 = dot(to_end, to_end);

    const float d2 = m0 - sqr(taper);
    const float k2 = d2 - m2 * m2;
    const float k1 = d2 * m3 - m1 * m2 + m2 * taper * begin_radius;
    const float k0 = d2 * m5 - m1 * m1 + m1 * taper * begin_radius * 2.0 - m0 * sqr(begin_radius);
    const float h = sqr(k1) - k0 * k2;
    if(h < 0.0) {
        return false;
    }

    const float t = (-sqrt(h) - k1) / k2;
    if(t > asfloat(tile_dist_bounds.y)) {
        return false;
    }

    const float y = m1 - begin_radius * taper + t * m2;
    if(y > 0.0 && y < d2) {
        return true;
    }

    const float h1 = sqr(m3) - m5 + sqr(begin_radius);
    const float h2 = sqr(m6) - m7 + sqr(end_radius);
    return max(h1, h2) > 0.0;
}


void cull_points() {
    const uint2 range = dispatch_range(point_count);

    for(uint i = range.x; i != range.y; ++i) {
        if(is_inside(point_lights[i])) {
            const uint light_index = InterlockedAddRet(tile_point_count, 1);
            if(light_index >= max_tile_lights) {
                break;
            }
            light_indices[light_index] = i;
        }
    }
}

void cull_spots() {
    const uint2 range = dispatch_range(spot_count);

    for(uint i = range.x; i != range.y; ++i) {
        if(is_inside(spot_lights[i])) {
            const uint light_index = InterlockedAddRet(tile_spot_count, 1) + tile_point_count;
            if(light_index >= max_tile_lights) {
                break;
            }
            light_indices[light_index] = i;
        }
    }
}

void cull_lights() {
    cull_points();

    GroupMemoryBarrierWithGroupSync();

    cull_spots();

    GroupMemoryBarrierWithGroupSync();
}


// -------------------------------- MAIN --------------------------------


[shader("compute")]
[numthreads(16, 16)]
void comp_main() {
    const int2 coord = int2(semantics.global_id.xy);
    const float2 image_size = float2(image_size(out_color).xy);
    const float2 uv = (semantics.global_id.xy + 0.5) / image_size;

    const float depth = in_depth[coord];

    const float3 world_pos = unproject(uv, depth, camera.cur.inv_view_proj);

    float3 view_dir = (camera.position - world_pos);
    const float view_dist = length(view_dir);
    view_dir /= view_dist;

    compute_tile_infos(image_size, view_dist, is_depth_far(depth) || is_depth_near(depth));

    cull_lights();

    if(is_depth_far(depth)) {
        return;
    }

    float3 irradiance = out_color[coord].rgb;

    const SurfaceInfo surface = decode_gbuffer({
        in_rt0[coord],
        in_rt1[coord],
    });


    // -------------------------------- POINTS --------------------------------
    const uint point_begin = 0;
    const uint point_end = min(max_tile_lights, tile_point_count);
    for(uint i = point_begin; i != point_end; ++i) {
        const PointLight light = point_lights[light_indices[i]];
        const AreaLightInfo area = karis_area_light(surface, light, world_pos, view_dir);
        const float att = attenuation(area.orig_light_dist * light.falloff, light.range * light.falloff, light.min_radius * light.falloff);

        if(att > 0.0) {
            const float3 radiance = light.color * att;
            irradiance += radiance * eval_lighting(alpha_corrected_surface(surface, area), view_dir, area.light_dir);
        }
    }

    // -------------------------------- SPOTS --------------------------------
    const uint spot_begin = tile_point_count;
    const uint spot_end = min(max_tile_lights, spot_begin + tile_spot_count);
    for(uint i = spot_begin; i != spot_end; ++i) {
        const SpotLight light = spot_lights[light_indices[i]];
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
            irradiance += radiance * eval_lighting(alpha_corrected_surface(surface, area), view_dir, area.light_dir);
        }
    }

#ifdef DEBUG
    {
        const float total_lights = float(tile_point_count + tile_spot_count);
        float3 heat = heat_spectrum(total_lights / 16.0f);
        heat = lerp(heat, float3(1.0) - heat, print_value(semantics.local_id.xy * 2.0, float2(0.0), float2(8.0, 15.0), total_lights, 2.0, 0.0));
        irradiance = lerp(heat, irradiance, 0.8);
    }
#endif

    out_color[coord] = float4(irradiance, 1.0);
}

