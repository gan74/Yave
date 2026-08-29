#include "lib/structs.slang"
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

// #define DEBUG_CELLS
// #define DEBUG_LOD

#define EMISSIVE

static const bool lerp_between_lods = false;
static const uint extra_sample_count = 8;


[[vk::binding(0)]]
RWTexture2D out_image;

[[vk::binding(1)]]
RWStructuredBuffer<uint> hash_table;

[[vk::binding(2)]]
RWStructuredBuffer<RTGICell> sum_table;

[[vk::binding(3)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(4)]]
Sampler2D<float> in_depth;

[[vk::binding(5)]]
Sampler2D<uint> in_rt1;

[[vk::binding(6)]]
ConstantBuffer<Camera> camera;

[[vk::binding(7)]]
SamplerCube in_envmap;

[[vk::binding(8)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(9)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(10)]]
RWStructuredBuffer<RTGIUpdate> updates;

[[vk::binding(11)]]
RWStructuredBuffer<uint> update_count;

[[vk::binding(12)]]
cbuffer Params {
    uint hash_size;
    uint frame_id;
    uint reset_hash;
    float lod_jitter_strength;

    float lod_dist;
    float base_cell_size;
    float pos_jitter_strength;
    float norm_jitter_strength;

    float target_samples;
    float min_ray_count;
    float max_ray_count;
    uint light_count;

    uint max_updates;
    uint padding_0;
    uint padding_1;
    uint padding_2;
};


[[vk::binding(0, 1)]]
Sampler2D all_textures[];



struct RTGIMaterial {
    float3 albedo;
    float3 emissive;
};

RTGIMaterial eval_material(uint instance_id) {
    const MaterialData material = materials[instance_id];
    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
    const uint emissive_id = material.texture_indices[uint(TextureSlots::Emissive)];
    return RTGIMaterial(
        all_textures[NonUniformResourceIndex(albedo_id)].SampleLevel(float2(0.5), 100.0).rgb * material.base_color_factor,
        all_textures[NonUniformResourceIndex(emissive_id)].SampleLevel(float2(0.5), 100.0).rgb * material.emissive_factor,
    );
}

uint timestamp_index(uint h) {
    return hash_size + h;
}

#ifdef RTGI_SH
void write_value(uint index, SH<float> sh_r, SH<float> sh_g, SH<float> sh_b, float w) {
    RTGICell cell = sum_table[index];
    cell.packed_r = (SH<float>.unpack(cell.packed_r) + sh_r).pack();
    cell.packed_g = (SH<float>.unpack(cell.packed_g) + sh_g).pack();
    cell.packed_b = (SH<float>.unpack(cell.packed_b) + sh_b).pack();
    cell.weight += w;
    sum_table[index] = cell;
}
#else
void write_value(uint index, float3 radiance, float w) {
    RTGICell cell = sum_table[index];
    cell.radiance += radiance;
    cell.weight += w;
    sum_table[index] = cell;
}
#endif

float3 jitter_position<T: Rng>(float3 normal, inout T rng) {
    const float3 v = rng.next_unit();
    return (v - normal * dot(v, normal));
}

void push_update(uint index, float3 position, float3 normal, uint lod) {
    uint prev;
    InterlockedExchange(hash_table[timestamp_index(index)], frame_id, prev);
    if(prev == frame_id) {
        return;
    }

    uint slot;
    InterlockedAdd(update_count[0], 1, slot);
    if(slot < max_updates) {
        updates[slot] = { position, index, normal, lod };
    }
}










#ifdef TRIM
[shader("compute")]
[numthreads(256)]
void main() {
    const uint h = semantics.global_index;

    if(h == 0) {
        update_count[0] = 0;
    }

    if(reset_hash > 0 || hash_table[timestamp_index(h)] == (frame_id - 4)) {
        hash_table[h] = 0;
#ifdef RTGI_SH
        sum_table[h].packed_r = uint4(0);
        sum_table[h].packed_g = uint4(0);
        sum_table[h].packed_b = uint4(0);
#else
        sum_table[h].radiance = float3(0.0);
#endif
        sum_table[h].weight = 0.0;
    } else {
        const float weight = sum_table[h].weight;
        if(weight > 2.0 * target_samples) {
            const float norm_factor = target_samples / weight;
#ifdef RTGI_SH
            sum_table[h].packed_r = (SH<float>.unpack(sum_table[h].packed_r) * norm_factor).pack();
            sum_table[h].packed_g = (SH<float>.unpack(sum_table[h].packed_g) * norm_factor).pack();
            sum_table[h].packed_b = (SH<float>.unpack(sum_table[h].packed_b) * norm_factor).pack();
#else
            sum_table[h].radiance *= norm_factor;
#endif
            sum_table[h].weight = target_samples;
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

    const float float_lod_level = HashKey.float_lod_level(view_dist, lod_dist) + rng.next_unorm() * lod_jitter_strength;
    const float lod_blend = frac(float_lod_level);
    const uint lod_level = uint(float_lod_level);

    const float3 base_pos_jitter = jitter_position(normal, rng) * pos_jitter_strength;
#ifdef RTGI_SH
    const HashKey base_key = HashKey.make_key(world_pos, base_cell_size, lod_level, base_pos_jitter);
    const HashKey base_key_lo = HashKey.make_key(world_pos, base_cell_size, lod_level + 1, base_pos_jitter * 0.5);
#else
    const float3 base_norm_jitter = rng.next_unit() * norm_jitter_strength;
    const HashKey base_key = HashKey.make_key(world_pos, normal, base_cell_size, lod_level, base_pos_jitter, base_norm_jitter);
    const HashKey base_key_lo = HashKey.make_key(world_pos, normal, base_cell_size, lod_level + 1, base_pos_jitter * 0.5, base_norm_jitter);
#endif

#ifdef UPDATE
    bool inserted;
    const uint base_index = find_update_bucket_index(hash_table, base_key, hash_size, inserted);
    if(base_index == constants.max_uint) {
        return;
    }

    push_update(base_index, world_pos, normal, lod_level);

    if(lerp_between_lods) {
        const uint index = find_update_bucket_index(hash_table, base_key_lo, hash_size, inserted);
        if(index != constants.max_uint) {
            push_update(index, world_pos, normal, lod_level + 1);
        }
    }
#endif

#ifdef APPLY
    {
        DebugPrinter dbg = DebugPrinter(coord);
        dbg.puts(CHAR_U, CHAR_P, CHAR_D, CHAR_COLON, CHAR_SPACE, CHAR_SPACE);
        dbg.putf(min(update_count[0], max_updates), 6.0, 0.0);
        if(dbg.value > 0.0) {
            out_image[coord] = float4(1.0);
            return;
        }
    }

    const uint base_index = find_bucket_index(hash_table, base_key, hash_size);
    if(base_index == constants.max_uint) {
        return;
    }

    const RTGICell base_cell = sum_table[base_index];
#ifdef RTGI_SH
    SH<float> acc_r = SH<float>.unpack(base_cell.packed_r);
    SH<float> acc_g = SH<float>.unpack(base_cell.packed_g);
    SH<float> acc_b = SH<float>.unpack(base_cell.packed_b);
#else
    float3 acc = sum_table[base_index].radiance;
#endif
    float acc_w = base_cell.weight;

    if(lerp_between_lods) {
        const uint index = find_bucket_index(hash_table, base_key_lo, hash_size);
        if(index != constants.max_uint) {
            const RTGICell lo = sum_table[index];
#ifdef RTGI_SH
            acc_r = acc_r + (SH<float>.unpack(lo.packed_r) - acc_r) * lod_blend;
            acc_g = acc_g + (SH<float>.unpack(lo.packed_g) - acc_g) * lod_blend;
            acc_b = acc_b + (SH<float>.unpack(lo.packed_b) - acc_b) * lod_blend;
#else
            acc = lerp(acc, lo.radiance, lod_blend);
#endif
            acc_w = lerp(acc_w, lo.weight, lod_blend);
        }
    }

    for(uint k = 0; k != extra_sample_count; ++k) {
        const float3 pos_jitter = jitter_position(normal, rng) * pos_jitter_strength;
#ifdef RTGI_SH
        const HashKey key = HashKey.make_key(world_pos, base_cell_size, lod_level, pos_jitter);
#else
        const float3 norm_jitter = rng.next_unit() * norm_jitter_strength;
        const HashKey key = HashKey.make_key(world_pos, normal, base_cell_size, lod_level, pos_jitter, norm_jitter);
#endif
        if(key != base_key) {
            const uint index = find_bucket_index(hash_table, key, hash_size);
            if(index != constants.max_uint) {
                const RTGICell cell = sum_table[index];
#ifdef RTGI_SH
                acc_r = acc_r + SH<float>.unpack(cell.packed_r);
                acc_g = acc_g + SH<float>.unpack(cell.packed_g);
                acc_b = acc_b + SH<float>.unpack(cell.packed_b);
#else
                acc += cell.radiance;
#endif
                acc_w += cell.weight;
            }
        }
    }

    const float w = max(1.0, acc_w);
#ifdef RTGI_SH
    out_image[coord] = float4(max(float3(0.0), float3(
        acc_r.eval_hemisphere_cosine_weighted(normal),
        acc_g.eval_hemisphere_cosine_weighted(normal),
        acc_b.eval_hemisphere_cosine_weighted(normal)
    ) / w), 1.0);
#else
    out_image[coord] = float4(max(0.0, acc / w), w);
#endif

#ifdef DEBUG_CELLS
    {
        out_image[coord] = float4(identifying_color(base_index) * 0.33, 1.0);
    }
#endif

#ifdef DEBUG_LOD
    {
        out_image[coord] = float4(frac(float_lod_level).xxx, 1.0);
        // out_image[coord] = float4(lerp(identifying_color(lod_level), identifying_color(lod_level + 1), frac(float_lod_level)) * 0.33, 1.0);
    }
#endif

#endif
}
#endif


#ifdef TRACE
[shader("compute")]
[numthreads(64)]
void main() {
    const uint i = semantics.global_index;
    if(i >= max_updates || i >= update_count[0]) {
        return;
    }

    const RTGIUpdate update = updates[i];

    const uint seed = hash.xorshift(i + hash.xorshift(frame_id));
    HashRng rng = HashRng(seed);

    const float total_rays = 1024.0 * 1024.0;
    const uint ray_count = uint(clamp(round(total_rays / update_count[0]), min_ray_count, max_ray_count));

#ifdef RTGI_SH
    SH<float> sh_r;
    SH<float> sh_g;
    SH<float> sh_b;
#else
    float3 acc = float3(0.0);
#endif
    for(uint r = 0; r != ray_count; ++r) {
        const float tmin = 0.001;
        const float tmax = 10000.0;

#ifdef RTGI_SH
        const float3 trace_dir = rng.next_unit();
#else
        const float3 trace_dir = cosine_sample_hemisphere(update.normal, rng);
#endif
        const HitInfo hit = trace_inline(tlas, update.position, trace_dir, tmin, tmax);

        float3 radiance = float3(0.0);
        if(hit.hit) {
            const float3 hit_normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
            const RTGIMaterial material = eval_material(hit.instance_id);

#ifdef EMISSIVE
            radiance += material.emissive;
#endif

            radiance += material.albedo * in_envmap.SampleLevel(hit_normal, 999.0).rgb;
            for(uint k = 0; k != light_count; ++k) {
                const DirectionalLight light = lights[k];
                const float n_l = dot(light.direction, hit_normal);
                if(n_l > 0.0) {
                    if(light.cast_shadow == 0 || !trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
                        radiance += (material.albedo * light.color * n_l);
                    }
                }
            }
        } else {
            radiance += in_envmap.SampleLevel(trace_dir, 0.0).rgb;
        }

#ifdef RTGI_SH
        const SH<float> basis = SH<float>.basis(trace_dir);
        sh_r = sh_r + basis * radiance.r;
        sh_g = sh_g + basis * radiance.g;
        sh_b = sh_b + basis * radiance.b;
#else
        acc += radiance;
#endif
    }

#ifdef RTGI_SH
    write_value(update.index, sh_r, sh_g, sh_b, float(ray_count));
#else
    write_value(update.index, acc * constants.inv_pi, float(ray_count));
#endif
}
#endif
