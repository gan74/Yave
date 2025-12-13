#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/spatial_hash.slang"

#ifdef __cplusplus
#define TRIM
#define COUNT
#define UPDATE
#define APPLY
#endif

// #define DEBUG_CELLS
// #define DEBUG_RAY_COUNT
// #define DEBUG_LOD

// #define EMISSIVE
// #define BROADCAST_SAMPLES

static const bool lerp_between_lods = true;
static const uint extra_sample_count = 8;


[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2D<uint> in_rt1;

[[vk::binding(2)]]
ConstantBuffer<Camera> camera;

[[vk::binding(3)]]
RWStructuredBuffer<uint> hash_table;

[[vk::binding(4)]]
RWStructuredBuffer<float> sum_table;

[[vk::binding(5)]]
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
};


[[vk::binding(6)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(7)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(8)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(9)]]
SamplerCube in_envmap;

[[vk::binding(10)]]
RWTexture2D out_image;

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

uint count_index(uint h) {
    return hash_size * 2 + h;
}

uint samples_index(uint h) {
    return hash_size * 3 + h;
}



#ifdef TRIM
[shader("compute")]
[numthreads(64)]
void main() {
    const uint h = semantics.global_index;

    hash_table[count_index(h)] = 0;
    hash_table[samples_index(h)] = uint(sum_table[h * 4 + 3]);

    if(reset_hash > 0 || hash_table[timestamp_index(h)] == (frame_id - 4)) {
        hash_table[h] = 0;
        for(uint i = 0; i != 4; ++i) {
            sum_table[h * 4 + i] = 0.0;
        }
    } else {
        const float weight = sum_table[h * 4 + 3];

        if(weight > 2.0 * target_samples) {
            const float norm_factor = target_samples / weight;

            sum_table[h * 4 + 3] = target_samples;
            for(uint i = 0; i != 3; ++i) {
                sum_table[h * 4 + i] *= norm_factor;
            }
        }
    }
}
#endif


#if defined(COUNT) || defined(UPDATE) || defined(APPLY)

float4 read_value(uint index) {
    float4 acc;
    for(uint i = 0; i != 4; ++i) {
        acc[i] = sum_table[index * 4 + i];
    }
    return acc;
}

void write_value(uint index, float4 acc) {
    hash_table[timestamp_index(index)] = frame_id;
    for(uint i = 0; i != 4; ++i) {
        if(acc[i] > 0.0) {
            float prev;
            InterlockedAdd(sum_table[index * 4 + i], acc[i], prev);
        }
    }
}

float3 jitter_position<T: Rng>(float3 normal, inout T rng) {
    const float3 v = rng.next_unit();
    return (v - normal * dot(v, normal));
}

float compute_ray_count(uint index) {
    const uint threads_in_cell = hash_table[count_index(index)];
    const float samples_in_cell = float(hash_table[samples_index(index)]);
    const float samples_needed = max(0.0, target_samples - samples_in_cell);
    return clamp(samples_needed / threads_in_cell, min_ray_count, max_ray_count);
}


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
    const float3 base_norm_jitter = rng.next_unit() * norm_jitter_strength;
    const HashKey base_key = HashKey.make_key(world_pos, normal, base_cell_size, lod_level, base_pos_jitter, base_norm_jitter);
    const HashKey base_key_lo = HashKey.make_key(world_pos, normal, base_cell_size, lod_level + 1, base_pos_jitter * 0.5, base_norm_jitter);

#ifdef APPLY
    const uint base_index = find_bucket_index(hash_table, base_key, hash_size);
    if(base_index == constants.max_uint) {
        return;
    }

    float4 acc = read_value(base_index);

    if(lerp_between_lods) {
        const uint index = find_bucket_index(hash_table, base_key_lo, hash_size);
        if(index != constants.max_uint) {
            acc = lerp(acc, read_value(index), lod_blend);
        }
    }

    for(uint k = 0; k != extra_sample_count; ++k) {
        const float3 pos_jitter = jitter_position(normal, rng) * pos_jitter_strength;
        const float3 norm_jitter = rng.next_unit() * norm_jitter_strength;
        const HashKey key = HashKey.make_key(world_pos, normal, base_cell_size, lod_level, pos_jitter, norm_jitter);
        if(key != base_key) {
            const uint index = find_bucket_index(hash_table, key, hash_size);
            if(index != constants.max_uint) {
                acc += read_value(index);
            }
        }
    }

    out_image[coord] = acc / max(1.0, acc.a);

#ifdef DEBUG_CELLS
    {
        out_image[coord] = float4(identifying_color(base_index) * 0.33, 1.0);
    }
#endif

#ifdef DEBUG_RAY_COUNT
    {
        const float ray_count = compute_ray_count(base_index);
        const float t = saturate((ray_count - min_ray_count) / (max_ray_count - min_ray_count));
        out_image[coord] = float4(lerp(float3(0, 1, 0), float3(1, 0, 0), t), 1.0);
    }
#endif

#ifdef DEBUG_LOD
    {
        out_image[coord] = float4(frac(float_lod_level).xxx, 1.0);
        // out_image[coord] = float4(lerp(identifying_color(lod_level), identifying_color(lod_level + 1), frac(float_lod_level)) * 0.33, 1.0);
    }
#endif

#endif

#ifdef COUNT
    bool inserted;
    const uint base_index = find_update_bucket_index(hash_table, base_key, hash_size, inserted);
    if(base_index == constants.max_uint) {
        return;
    }

    if(lerp_between_lods) {
        find_update_bucket_index(hash_table, base_key_lo, hash_size, inserted);
    }

    uint prev;
    InterlockedAdd(hash_table[count_index(base_index)], 1, prev);
#endif


#ifdef UPDATE
    const uint base_index = find_bucket_index(hash_table, base_key, hash_size);
    if(base_index == constants.max_uint) {
        return;
    }

    const float float_ray_count = compute_ray_count(base_index);
    const uint ray_count = uint(float_ray_count) + (rng.next_unorm() < frac(float_ray_count) ? 1 : 0);
    if(ray_count == 0) {
        return;
    }

    float3 acc = float3(0.0);
    for(uint i = 0; i != ray_count; ++i) {
        const float3 trace_dir = cosine_sample_hemisphere(normal, rng);
        const float n_t = constants.inv_pi;

        const float tmin = 0.001;
        const float tmax = 10000.0;

        const HitInfo hit = trace_inline(tlas, world_pos, trace_dir, tmin, tmax);

        if(hit.hit) {
            const float3 hit_normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
            const RTGIMaterial material = eval_material(hit.instance_id);

#ifdef EMISSIVE
            acc.rgb += material.emissive;
#endif

            for(uint k = 0; k != light_count; ++k) {
                const DirectionalLight light = lights[k];
                const float n_l = dot(light.direction, hit_normal);
                if(n_l > 0.0) {
                    if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
                        acc += (material.albedo * light.color * n_l * n_t);
                    }
                }
            }
        } else {
            acc += in_envmap.SampleLevel(trace_dir, 0.0).rgb * n_t;
        }
    }

    write_value(base_index, float4(acc, ray_count));

    if(lerp_between_lods) {
        const uint index = find_bucket_index(hash_table, base_key_lo, hash_size);
        if(index != constants.max_uint) {
            write_value(index, float4(acc, ray_count));
        }
    }

#ifdef BROADCAST_SAMPLES
    for(uint k = 0; k != extra_sample_count; ++k) {
        const float3 pos_jitter = jitter_position(normal, rng) * pos_jitter_strength;
        const float3 norm_jitter = rng.next_unit() * norm_jitter_strength;
        const HashKey key = HashKey.make_key(world_pos, normal, base_cell_size, lod_level, pos_jitter, norm_jitter);

        if(key != base_key) {
            bool inserted;
            const uint index = find_update_bucket_index(hash_table, key, hash_size, inserted);
            if(index != constants.max_uint) {
                write_value(index, float4(acc, ray_count));
            }
        }
    }
#endif
#endif
}
#endif
