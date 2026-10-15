#include "lib/utils.slang"

// #define DEPTH_CULL

// -------------------------------- I/O --------------------------------

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2DShadow in_shadows;

[[vk::binding(2)]]
ConstantBuffer<Camera> camera;

[[vk::binding(3)]]
StructuredBuffer<PointLight> point_lights;

[[vk::binding(4)]]
StructuredBuffer<SpotLight> spot_lights;

[[vk::binding(5)]]
StructuredBuffer<ShadowMapInfo> shadow_infos;

[[vk::binding(6)]]
ConstantBuffer<LightClusterInfo> cluster_info;

[[vk::binding(7)]]
RWStructuredBuffer<LightTile> out_tiles;





// -------------------------------- SHARED --------------------------------

groupshared uint tile_point_count;
groupshared uint tile_spot_count;

groupshared float3 tile_dir;
groupshared float tile_sin_alpha;

groupshared uint2 tile_dist_bounds;




// -------------------------------- TILE INFOS --------------------------------

void compute_tile_culling_info(float2 image_size) {
    const float2 center_uv = ((semantics.work_group_id.xy + 0.5) * semantics.work_group_size.xy) / image_size;
    const float2 center_ndc = center_uv * 2.0 - 1.0;
    const float3 center_dir = normalize(unproject_ndc(float3(center_ndc, 1.0), camera.curr.inv_view_proj) - camera.position);

    const uint corner_index = semantics.local_index;
    const float2 corner_sign = float2((corner_index & 0x01) != 0 ? 1.0 : -1.0, (corner_index & 0x02) != 0 ? 1.0 : -1.0);
    const float2 half_tile_ndc = semantics.work_group_size.xy / image_size;
    const float3 corner_unproj = unproject_ndc(float3(center_ndc + corner_sign * half_tile_ndc, 1.0), camera.curr.inv_view_proj);
    const float3 corner_dir = normalize(corner_unproj - camera.position);

    const float tile_cos_alpha = WaveActiveMin(dot(center_dir, corner_dir));

    if(corner_index == 0) {
        tile_dir = center_dir;
        tile_sin_alpha = sqrt(1.0 - sqr(tile_cos_alpha));
    }
}



// -------------------------------- CULLING --------------------------------

bool is_sphere_inside(float3 center, float radius, float cos_alpha) {
    const float3 closest = camera.position + tile_dir * cos_alpha;
    const float tile_radius = cos_alpha * tile_sin_alpha;
    const float3 to_closest = closest - center;
    return dot(to_closest, to_closest) <= sqr(radius + tile_radius);
}

bool is_inside(PointLight light) {
    const float cos_alpha = max(0.0, dot(light.position - camera.position, tile_dir));

#ifdef DEPTH_CULL
    if(cos_alpha + light.range < asfloat(tile_dist_bounds.x) ||
       cos_alpha - light.range > asfloat(tile_dist_bounds.y)) {
        return false;
    }
#endif

    return is_sphere_inside(light.position, light.range, cos_alpha);
}

bool is_inside(SpotLight light) {
    const float cos_alpha = max(0.0, dot(light.encl_sphere_center - camera.position, tile_dir));

#ifdef DEPTH_CULL
    if(cos_alpha + light.encl_sphere_radius < asfloat(tile_dist_bounds.x) ||
       cos_alpha - light.encl_sphere_radius > asfloat(tile_dist_bounds.y)) {
        return false;
    }
#endif

    // https://iquilezles.org/articles/intersectors/ (rounded cone)
    // Wide cones:the apex is inside the end sphere, so the rounded cone degenerates into the enclosing sphere.
    const float cos_angle = sqrt(saturate(1.0 - sqr(light.sin_angle)));
    if(light.sin_angle > cos_angle * 0.9) {
        return is_sphere_inside(light.encl_sphere_center, light.encl_sphere_radius, cos_alpha);
    }

    const float tile_radius = (cos_alpha + light.encl_sphere_radius) * tile_sin_alpha;
    const float3 begin = light.position;
    const float3 end = light.position + light.forward * (light.range * cos_angle);
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
    for(uint i = semantics.local_index; i < cluster_info.point_count; i += semantics.work_group_thread_count) {
        if(is_inside(point_lights[i])) {
            uint light_index = 0;
            InterlockedAdd(tile_point_count, 1, light_index);
            if(light_index >= max_light_per_tile) {
                break;
            }
            out_tiles[semantics.work_group_index].indices[light_index] = uint16_t(i);
        }
    }
}

void cull_spots() {
    for(uint i = semantics.local_index; i < cluster_info.spot_count; i += semantics.work_group_thread_count) {
        if(is_inside(spot_lights[i])) {
            uint light_index = 0;
            InterlockedAdd(tile_spot_count, 1, light_index);
            light_index += tile_point_count;
            if(light_index >= max_light_per_tile) {
                break;
            }
            out_tiles[semantics.work_group_index].indices[light_index] = uint16_t(i);
        }
    }
}


// -------------------------------- MAIN --------------------------------

[shader("compute")]
[numthreads(light_cluster_tile_size, light_cluster_tile_size)]
void comp_main() {
    const uint2 coord = semantics.global_id.xy;
    const float2 image_size = float2(image_size(in_depth).xy);
    const float2 uv = (semantics.global_id.xy + 0.5) / image_size;

    if(semantics.local_index < 4) {
        compute_tile_culling_info(image_size);
    }

    if(semantics.local_index == 0) {
        tile_point_count = 0;
        tile_spot_count = 0;
        tile_dist_bounds =  uint2(asuint(constants.max_float), 0);
    }

    GroupMemoryBarrierWithGroupSync();

    if(all(float2(coord) < image_size)) {
        const float depth = in_depth[coord];
        if(!is_depth_far(depth)) {
            const float view_dist = is_depth_near(depth) ? 0.0 : length(camera.position - unproject(uv, depth, camera.curr.inv_view_proj));
            const uint ui_dist = asuint(view_dist);
            InterlockedMin(tile_dist_bounds.x, ui_dist);
            InterlockedMax(tile_dist_bounds.y, ui_dist);
        }

    }

    GroupMemoryBarrierWithGroupSync();

    cull_points();

    GroupMemoryBarrierWithGroupSync();

    cull_spots();

    GroupMemoryBarrierWithGroupSync();

    if(semantics.local_index == 0) {
        out_tiles[semantics.work_group_index].point_count = uint16_t(tile_point_count);
        out_tiles[semantics.work_group_index].spot_count = uint16_t(tile_spot_count);
    }
}

