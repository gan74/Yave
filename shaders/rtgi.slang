#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/spatial_hash.slang"

#ifdef __cplusplus
#define TRIM
#define UPDATE
#define APPLY
#endif

// #define EMISSIVE


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
    float lod_dist;
    float base_cell_size;
    uint frame_id;

    uint reset_hash;
    float max_samples;
    uint max_ray_count;
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



#ifdef TRIM
[shader("compute")]
[numthreads(64)]
void main() {
    const uint h = semantics.global_index;

    if(reset_hash > 0 || hash_table[timestamp_index(h)] == (frame_id - 4)) {
        hash_table[h] = 0;
        for(uint i = 0; i != 4; ++i) {
            sum_table[h * 4 + i] = 0.0;
        }
    } else {
        const float weight = sum_table[h * 4 + 3];

        if(weight > 2.0 * max_samples) {
            const float norm_factor = max_samples / weight;

            sum_table[h * 4 + 3] = max_samples;
            for(uint i = 0; i != 3; ++i) {
                sum_table[h * 4 + i] *= norm_factor;
            }
        }
    }
}
#endif


#if defined(UPDATE) || defined(APPLY)

static const uint written_lod_count = 3;

float4 value_at(uint index) {
    float4 acc;
    for(uint i = 0; i != 4; ++i) {
        acc[i] = sum_table[index * 4 + i];
    }
    return acc;
}

float3 jitter_position<T: Rng>(float3 normal, inout T rng) {
    const float3 v = rng.next_unit();
    return (v - normal * dot(v, normal)) * 2.0;
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

    const uint lod_level = HashKey.lod_level(view_dist, lod_dist);
    const float3 jitter = jitter_position(normal, rng);

#ifdef APPLY
    float4 acc = float4(0.0);

    for(uint k = 0; k != written_lod_count; ++k) {
        const HashKey lod_key = HashKey.make_key(world_pos, normal, base_cell_size, lod_level + k, jitter);

        const uint index = find_bucket_index(hash_table, lod_key, hash_size);
        if(index != constants.max_uint) {
            acc += value_at(index);
        }
    }

    out_image[coord] = acc / max(1.0, acc.a);
#endif

#ifdef UPDATE
    float4 acc = float4(0.0);
    const uint ray_count = max_ray_count;

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
                        acc.rgb += (material.albedo * light.color * n_l * n_t);
                    }
                }
            }
        } else {
            acc.rgb += in_envmap.SampleLevel(trace_dir, 0.0).rgb * n_t;
        }

        acc.a += 1.0;
    }

    for(uint k = 0; k != written_lod_count; ++k) {
        const HashKey lod_key = HashKey.make_key(world_pos, normal, base_cell_size, lod_level + k, jitter);

        bool inserted;
        const uint index = find_update_bucket_index(hash_table, lod_key, hash_size, inserted);
        if(index == constants.max_uint) {
            continue;
        }

        hash_table[timestamp_index(index)] = frame_id;

        for(uint i = 0; i != 4; ++i) {
            if(acc[i] > 0.0) {
                float prev;
                InterlockedAdd(sum_table[index * 4 + i], acc[i], prev);
            }
        }
    }
#endif
}
#endif
