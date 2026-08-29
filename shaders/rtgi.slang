#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/spatial_hash.slang"
#include "lib/sh.slang"
#include "lib/debug.slang"

#ifdef __cplusplus
#define TRIM
#define UPDATE
#define TRACE
#define APPLY
#endif

#define DEBUG

struct RTGICount {
    uint updates;
    uint failled_allocs;
    uint padding_0;
    uint padding_1;
};



[[vk::binding(0)]]
RWTexture2D out_image;

[[vk::binding(1)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D<uint> in_rt1;

[[vk::binding(4)]]
ConstantBuffer<Camera> camera;

[[vk::binding(5)]]
SamplerCube in_envmap;

[[vk::binding(6)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(7)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(8)]]
RWStructuredBuffer<uint> hash_table;

[[vk::binding(9)]]
RWStructuredBuffer<RTGICell> cells;

[[vk::binding(10)]]
RWStructuredBuffer<RTGICellUpdate> cell_updates;

[[vk::binding(11)]]
RWStructuredBuffer<RTGICount> counts;

[[vk::binding(12)]]
cbuffer Params {
    uint hash_size;
    uint frame_id;
    uint reset_hash;
    float lod_dist;

    float base_cell_size;
    float target_weight;
    uint light_count;
    uint max_cell_updates;

    uint max_cell_count;
    uint padding_0;
    uint padding_1;
    uint padding_2;
};



[[vk::binding(0, 1)]]
Sampler2D all_textures[];





uint timestamp_index(uint h) {
    return hash_size + h;
}

uint sh_index_offset(uint h) {
    return hash_size * 2 + h;
}

void pack_sh(uint cell_index, SH<float3> sh) {
    const SH<float> r = SH<float>(sh.Y00.r, sh.Y11.r, sh.Y10.r, sh.Y1_1.r, sh.Y21.r, sh.Y2_1.r, sh.Y2_2.r, sh.Y20.r, sh.Y22.r);
    const SH<float> g = SH<float>(sh.Y00.g, sh.Y11.g, sh.Y10.g, sh.Y1_1.g, sh.Y21.g, sh.Y2_1.g, sh.Y2_2.g, sh.Y20.g, sh.Y22.g);
    const SH<float> b = SH<float>(sh.Y00.b, sh.Y11.b, sh.Y10.b, sh.Y1_1.b, sh.Y21.b, sh.Y2_1.b, sh.Y2_2.b, sh.Y20.b, sh.Y22.b);
    cells[cell_index].packed_r = r.pack();
    cells[cell_index].packed_g = g.pack();
    cells[cell_index].packed_b = b.pack();

}

SH<float3> unpack_sh(uint cell_index) {
    const SH<float> r = SH<float>.unpack(cells[cell_index].packed_r);
    const SH<float> g = SH<float>.unpack(cells[cell_index].packed_g);
    const SH<float> b = SH<float>.unpack(cells[cell_index].packed_b);
    return SH<float3>(
        float3(r.Y00,  g.Y00,  b.Y00),
        float3(r.Y11,  g.Y11,  b.Y11),
        float3(r.Y10,  g.Y10,  b.Y10),
        float3(r.Y1_1, g.Y1_1, b.Y1_1),
        float3(r.Y21,  g.Y21,  b.Y21),
        float3(r.Y2_1, g.Y2_1, b.Y2_1),
        float3(r.Y2_2, g.Y2_2, b.Y2_2),
        float3(r.Y20,  g.Y20,  b.Y20),
        float3(r.Y22,  g.Y22,  b.Y22)
    );
}

float3 shade_hit(HitInfo hit) {
    const float3 hit_normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
    const MaterialData material = materials[hit.instance_id];
    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
    const uint emissive_id = material.texture_indices[uint(TextureSlots::Emissive)];
    const float3 albedo = all_textures[NonUniformResourceIndex(albedo_id)].SampleLevel(float2(0.5), 100.0).rgb * material.base_color_factor;
    const float3 emissive = all_textures[NonUniformResourceIndex(emissive_id)].SampleLevel(float2(0.5), 100.0).rgb * material.emissive_factor;

    float3 radiance = albedo * in_envmap.SampleLevel(hit_normal, 999.0).rgb + emissive;
    for(uint k = 0; k != light_count; ++k) {
        const DirectionalLight light = lights[k];
        const float n_l = dot(light.direction, hit_normal);
        if(n_l > 0.0) {
            if(light.cast_shadow == 0 || !trace_inline(tlas, hit.position, light.direction, 0.001, 10000.0).hit) {
                radiance += albedo * light.color * n_l;
            }
        }
    }

    return radiance;
}

float4 eval_cell(uint cell_index, float3 normal) {
    if(cell_index != constants.max_uint) {
        const float w = cells[cell_index].weight;
        if(w >= 1.0) {
            return float4(max(0.0, unpack_sh(cell_index).eval_hemisphere_cosine_weighted(normal)) * w, w);
        }
    }
    return float4(0.0);
}

bool try_alloc_cell(float3 world_pos, uint lod, uint hash_index, inout HashRng rng) {
    for(uint k = 0; k != 8; ++k) {
        const uint test_index = (rng.next() & (max_cell_count - 1));

        uint prev;
        InterlockedCompareExchange(cells[test_index].hash_index, constants.max_uint, hash_index, prev);

        if(prev == constants.max_uint) {
            hash_table[sh_index_offset(hash_index)] = test_index;

            const HashKey lod_key = HashKey.make_key(world_pos, base_cell_size, lod + 1);
            const uint lod_index = find_bucket_index(hash_table, lod_key, hash_size);
            if(lod_index != constants.max_uint) {
                const uint lod_cell_index = hash_table[sh_index_offset(lod_index)];
                if(lod_cell_index != constants.max_uint) {
                    cells[test_index].packed_r = cells[lod_cell_index].packed_r;
                    cells[test_index].packed_g = cells[lod_cell_index].packed_g;
                    cells[test_index].packed_b = cells[lod_cell_index].packed_b;
                    cells[test_index].weight = cells[lod_cell_index].weight * 0.5;
                }
            }

            return true;
        }
    }

    return false;
}





#ifdef TRIM
[shader("compute")]
[numthreads(256)]
void main() {
    const uint h = semantics.global_index;

    if(h == 0) {
        counts[0].updates = 0;
        if(reset_hash != 0) {
            counts[0].failled_allocs = 0;
        }
    }

    if(reset_hash != 0) {
        hash_table[h] = 0;
        hash_table[sh_index_offset(h)] = constants.max_uint;

        if(h < max_cell_count) {
            cells[h] = RTGICell();
        }
    } else if(hash_table[timestamp_index(h)] == (frame_id - 4)) {
        hash_table[h] = 0;
        const uint cell_index = hash_table[sh_index_offset(h)];
        if(cell_index != constants.max_uint) {
            hash_table[sh_index_offset(h)] = constants.max_uint;
            cells[cell_index] = RTGICell();
        }
    }
}
#endif


#if defined(UPDATE) || defined(APPLY)
[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(in_depth);

    if(any(coord >= size)) {
        return;
    }

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        return;
    }

    const uint seed = hash.xorshift(semantics.global_index + hash.xorshift(frame_id));
    HashRng rng = HashRng(seed);

    const float2 uv = (coord + 0.5) / float2(size);
    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const float3 normal = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[coord])).normal;
    const float view_dist = length(world_pos - camera.position);
    const float lod = HashKey.float_lod_level(view_dist, lod_dist);

#ifdef UPDATE
    const uint lod_offset = 0; // rng.next_unorm() < frac(lod) ? 1 : 0;
    const HashKey key = HashKey.make_key(world_pos, base_cell_size, uint(lod) + lod_offset);

    bool inserted;
    const uint index = find_update_bucket_index(hash_table, key, hash_size, inserted);
    if(index == constants.max_uint) {
        return;
    }

    if(inserted) {
        if(!try_alloc_cell(world_pos, uint(lod), index, rng)) {
            uint prev;
            InterlockedAdd(counts[0].failled_allocs, 1, prev);
            return;
        }
    }

    uint prev_timestamp;
    InterlockedExchange(hash_table[timestamp_index(index)], frame_id, prev_timestamp);
    if(prev_timestamp != frame_id) {
        uint update_index;
        InterlockedAdd(counts[0].updates, 1, update_index);
        if(update_index < max_cell_updates) {
            cell_updates[update_index].index = index;
            cell_updates[update_index].world_pos = world_pos;
        }
    }
#endif

#ifdef APPLY

#ifdef DEBUG
    {
        DebugPrinter dbg = DebugPrinter(coord);
        dbg.puts(CHAR_U, CHAR_P, CHAR_D, CHAR_COLON, CHAR_SPACE, CHAR_SPACE);
        dbg.putf(min(counts[0].updates, max_cell_updates), 6.0, 0.0);
        dbg.next_line();
        dbg.puts(CHAR_A, CHAR_F, CHAR_L, CHAR_D, CHAR_COLON, CHAR_SPACE);
        dbg.putf(counts[0].failled_allocs, 6.0, 0.0);
        if(dbg.value > 0.0) {
            out_image[coord] = float4(1.0);
            return;
        }
    }
#endif

    const HashKey base_key = HashKey.make_key(world_pos, base_cell_size, uint(lod));
    const uint base_index = find_bucket_index(hash_table, base_key, hash_size);
    if(base_index == constants.max_uint) {
        return;
    }

    float4 gi = eval_cell(hash_table[sh_index_offset(base_index)], normal);
    const uint extra_sample_count = 32;
    for(uint k = 0; k != 2; ++k) {
        for(uint i = 0; i != extra_sample_count; ++i) {
            const HashKey key = HashKey.make_key(world_pos, base_cell_size, uint(lod) + k, rng.next_unit());
            const uint index = find_bucket_index(hash_table, key, hash_size);
            if(index != constants.max_uint) {
                gi += eval_cell(hash_table[sh_index_offset(index)], normal);
            }
        }
}

    out_image[coord] = gi.a < 1.0 ? float4(0.0) : float4(gi.rgb / gi.a, 1.0);

    /*const uint cell_index = hash_table[sh_index_offset(base_index)];
    if(cell_index != constants.max_uint) {
        RTGICell cc = cells[cell_index];
        out_image[coord] = float4(heat_spectrum(saturate(cc.weight / target_weight)), 1.0);
    }*/
#endif
}
#endif


#ifdef TRACE
[shader("compute")]
[numthreads(64)]
void main() {
    const uint index = semantics.global_index;
    const uint update_count = min(counts[0].updates, max_cell_updates);
    if(index >= update_count) {
        return;
    }

    const uint seed = hash.xorshift(index + hash.xorshift(frame_id));
    HashRng rng = HashRng(seed);

    const float total_rays = 1024.0 * 1024.0;
    const float rays_per_update = min(128.0, total_rays / update_count);
    const uint sample_count = uint(rays_per_update) + (rng.next_unorm() < frac(rays_per_update) ? 1 : 0);
    if(sample_count == 0) {
        return;
    }

    const RTGICellUpdate cell = cell_updates[index];
    const uint cell_index = hash_table[sh_index_offset(cell.index)];
    if(cell_index == constants.max_uint) {
        return;
    }

    SH<float3> sum;
    for(uint k = 0; k != sample_count; ++k) {
        const float3 trace_dir = rng.next_unit();
        const HitInfo hit = trace_inline(tlas, cell.world_pos, trace_dir, 0.001, 10000.0);
        const float3 radiance = hit.hit ? shade_hit(hit) : in_envmap.SampleLevel(trace_dir, 0.0).rgb;
        sum = sum + (SH<float>.basis(trace_dir) * radiance);
    }


#if 1
    const float prev_w = cells[cell_index].weight;
    const SH<float3> prev_sh = unpack_sh(cell_index) * prev_w;

    const float new_w = (prev_w + sample_count);
    const SH<float3> new_sh = (prev_sh + sum) / new_w;
#else
    const float prev_w = cells[cell_index].weight;
    const SH<float3> prev_sh = unpack_sh(cell_index);

    const float new_w = prev_w + 1.0;
    const SH<float3> contribution = sum * (1.0 / sample_count);
    const SH<float3> new_sh = prev_sh + (contribution - prev_sh) * (1.0 / new_w);
#endif

    pack_sh(cell_index, new_sh);
    cells[cell_index].weight = new_w;
}
#endif
