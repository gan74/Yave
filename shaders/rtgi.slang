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

static const uint extra_lods = 1;
static const uint apply_sample_count = 8;
static const float apply_lum_sigma = 1.0; // log2
static const float apply_lum_epsilon = 1e-4;
static const float emissive_scale = 1.0;


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
        all_textures[NonUniformResourceIndex(emissive_id)].SampleLevel(float2(0.5), 100.0).rgb * material.emissive_factor * emissive_scale,
    );
}

uint timestamp_index(uint h) {
    return hash_size + h;
}

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










#if  defined(TRIM)
[shader("compute")]
[numthreads(256)]
void main() {
    const uint h = semantics.global_index;

    if(h == 0) {
        update_count[0] = 0;
    }

    if(reset_hash > 0 || hash_table[timestamp_index(h)] == (frame_id - 4)) {
        hash_table[h] = 0;
        sum_table[h].radiance = float3(0.0);
        sum_table[h].weight = 0.0;
    } else {
        const float weight = sum_table[h].weight;
        if(weight > 2.0 * target_samples) {
            const float norm_factor = target_samples / weight;
            sum_table[h].radiance *= norm_factor;
            sum_table[h].weight = target_samples;
        }
    }
}
#endif







#if defined(UPDATE)
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
    const uint lod_level = uint(float_lod_level);

    for(uint k = 0; k <= extra_lods; ++k) {
        const HashKey key = HashKey.make_key(world_pos, normal, base_cell_size, lod_level + k);
        bool inserted;
        const uint index = find_update_bucket_index(hash_table, key, hash_size, inserted);
        if(index != constants.max_uint) {
            push_update(index, world_pos, normal, lod_level + k);
        }
    }
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

    float3 acc = float3(0.0);
    for(uint r = 0; r != ray_count; ++r) {
        const float tmin = 0.001;
        const float tmax = 10000.0;

        const float3 trace_dir = cosine_sample_hemisphere(update.normal, rng);
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
        acc += radiance;
    }

    RTGICell cell = sum_table[update.index];
    cell.radiance += acc * constants.inv_pi;
    cell.weight += float(ray_count);
    sum_table[update.index] = cell;
}
#endif














#if defined(APPLY)
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
    const uint lod_level = uint(float_lod_level);

    {
        DebugPrinter dbg = DebugPrinter(coord);
        dbg.puts(CHAR_U, CHAR_P, CHAR_D, CHAR_COLON, CHAR_SPACE, CHAR_SPACE);
        dbg.putf(min(update_count[0], max_updates), 6.0, 0.0);
        if(dbg.value > 0.0) {
            out_image[coord] = float4(1.0);
            return;
        }
    }

    float3 acc = float3(0.0);

    float total_weight = 0.0;
    float own_lum = -1.0;
    for(uint k = 0; k <= extra_lods; ++k) {
        for(uint i = 0; i != apply_sample_count; ++i) {
            // First sample uses the unjittered key that UPDATE inserted for this pixel, so at least one lookup always hits
            const bool jitter = i != 0;
            const float3 pos_jitter = jitter ? jitter_position(normal, rng) * pos_jitter_strength : float3(0.0);
            const float3 norm_jitter = jitter ? rng.next_unit() * norm_jitter_strength : float3(0.0);
            const HashKey key = HashKey.make_key(world_pos, normal, base_cell_size, lod_level + k, pos_jitter, norm_jitter);
            const uint index = find_bucket_index(hash_table, key, hash_size);
            if(index == constants.max_uint) {
                continue;
            }
            const RTGICell cell = sum_table[index];
            if(cell.weight < 1.0) {
                continue;
            }

            const float cell_lum = luminance(cell.radiance / cell.weight);
            if(own_lum < 0.0) {
                own_lum = cell_lum;
            }
            const float ratio = log2((cell_lum + apply_lum_epsilon) / (own_lum + apply_lum_epsilon));
            const float w = cell.weight * exp(-ratio * ratio / (2.0 * sqr(apply_lum_sigma)));
            total_weight += w;
            acc += cell.radiance * (w / cell.weight);
        }
    }

    out_image[coord] = float4(total_weight > 0.0 ? max(0.0, acc / total_weight) : float3(0.0), 1.0);

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
}
#endif
