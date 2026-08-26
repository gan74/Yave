#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/spatial_hash.slang"
#include "lib/sh.slang"

#ifdef __cplusplus
#define TRIM
#define UPDATE
#define APPLY
#endif

static const uint sh_stride = 28; // 9 * 3 (RGB SH coefficients) + 1 (weight)

[[vk::binding(0)]]
RWTexture2D out_image;

[[vk::binding(1)]]
RWStructuredBuffer<uint> hash_table;

[[vk::binding(2)]]
RWStructuredBuffer<float> sh_table;

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
cbuffer Params {
    uint hash_size;
    uint frame_id;
    uint reset_hash;
    float lod_dist;

    float base_cell_size;
    float target_weight;
    uint light_count;
    uint padding_0;
};

[[vk::binding(0, 1)]]
Sampler2D all_textures[];






uint timestamp_index(uint h) {
    return hash_size + h;
}

uint sh_offset(uint h) {
    return h * sh_stride;
}

uint weight_offset(uint h) {
    return h * sh_stride + 27;
}

bool read_sh(uint h, out SH<float3> sh) {
    const float weight = sh_table[weight_offset(h)];
    if(weight < 1.0) {
        sh = SH<float3>();
        return false;
    }

    const uint base = sh_offset(h);
    sh = SH<float3>(
        float3(sh_table[base +  0], sh_table[base +  1], sh_table[base +  2]),
        float3(sh_table[base +  3], sh_table[base +  4], sh_table[base +  5]),
        float3(sh_table[base +  6], sh_table[base +  7], sh_table[base +  8]),
        float3(sh_table[base +  9], sh_table[base + 10], sh_table[base + 11]),
        float3(sh_table[base + 12], sh_table[base + 13], sh_table[base + 14]),
        float3(sh_table[base + 15], sh_table[base + 16], sh_table[base + 17]),
        float3(sh_table[base + 18], sh_table[base + 19], sh_table[base + 20]),
        float3(sh_table[base + 21], sh_table[base + 22], sh_table[base + 23]),
        float3(sh_table[base + 24], sh_table[base + 25], sh_table[base + 26])
    ) / weight;

    return true;
}

void atomic_add_sh(uint h, SH<float3> sh) {
    const uint base = sh_offset(h);
    float prev;

    InterlockedAdd(sh_table[base +  0], sh.Y00.r, prev);
    InterlockedAdd(sh_table[base +  1], sh.Y00.g, prev);
    InterlockedAdd(sh_table[base +  2], sh.Y00.b, prev);

    InterlockedAdd(sh_table[base +  3], sh.Y11.r, prev);
    InterlockedAdd(sh_table[base +  4], sh.Y11.g, prev);
    InterlockedAdd(sh_table[base +  5], sh.Y11.b, prev);

    InterlockedAdd(sh_table[base +  6], sh.Y10.r, prev);
    InterlockedAdd(sh_table[base +  7], sh.Y10.g, prev);
    InterlockedAdd(sh_table[base +  8], sh.Y10.b, prev);

    InterlockedAdd(sh_table[base +  9], sh.Y1_1.r, prev);
    InterlockedAdd(sh_table[base + 10], sh.Y1_1.g, prev);
    InterlockedAdd(sh_table[base + 11], sh.Y1_1.b, prev);

    InterlockedAdd(sh_table[base + 12], sh.Y21.r, prev);
    InterlockedAdd(sh_table[base + 13], sh.Y21.g, prev);
    InterlockedAdd(sh_table[base + 14], sh.Y21.b, prev);

    InterlockedAdd(sh_table[base + 15], sh.Y2_1.r, prev);
    InterlockedAdd(sh_table[base + 16], sh.Y2_1.g, prev);
    InterlockedAdd(sh_table[base + 17], sh.Y2_1.b, prev);

    InterlockedAdd(sh_table[base + 18], sh.Y2_2.r, prev);
    InterlockedAdd(sh_table[base + 19], sh.Y2_2.g, prev);
    InterlockedAdd(sh_table[base + 20], sh.Y2_2.b, prev);

    InterlockedAdd(sh_table[base + 21], sh.Y20.r, prev);
    InterlockedAdd(sh_table[base + 22], sh.Y20.g, prev);
    InterlockedAdd(sh_table[base + 23], sh.Y20.b, prev);

    InterlockedAdd(sh_table[base + 24], sh.Y22.r, prev);
    InterlockedAdd(sh_table[base + 25], sh.Y22.g, prev);
    InterlockedAdd(sh_table[base + 26], sh.Y22.b, prev);

    InterlockedAdd(sh_table[base + 27], 1.0, prev);
}








#ifdef TRIM
[shader("compute")]
[numthreads(64)]
void main() {
    const uint h = semantics.global_index;

    if(reset_hash > 0 || hash_table[timestamp_index(h)] == (frame_id - 4)) {
        hash_table[h] = 0;
        for(uint i = 0; i != sh_stride; ++i) {
            sh_table[h * sh_stride + i] = 0.0;
        }
    } else {
        const float weight = sh_table[weight_offset(h)];
        if(weight > 2.0 * target_weight) {
            const float scale = target_weight / weight;
            for(uint i = 0; i != sh_stride; ++i) {
                sh_table[h * sh_stride + i] *= scale;
            }
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

    const float2 uv = (coord + 0.5) / float2(size);
    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const float3 normal = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[coord])).normal;
    const float view_dist = length(world_pos - camera.position);
    const uint lod = HashKey.lod_level(view_dist, lod_dist);
    const HashKey key = HashKey.make_key_pos_only(world_pos, base_cell_size, lod);

#ifdef UPDATE
    bool inserted;
    const uint index = find_update_bucket_index(hash_table, key, hash_size, inserted);
    if(index == constants.max_uint) {
        return;
    }
	
    hash_table[timestamp_index(index)] = frame_id;

    const uint seed = hash.xorshift(semantics.global_index + hash.xorshift(frame_id));
    HashRng rng = HashRng(seed);

    // const float3 trace_dir = cosine_sample_hemisphere(normal, rng);
    const float3 trace_dir = rng.next_unit();
    const HitInfo hit = trace_inline(tlas, world_pos, trace_dir, 0.001, 10000.0);

    float3 radiance = float3(0.0);
    if(hit.hit) {
        const float3 hit_normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
        const MaterialData material = materials[hit.instance_id];
        const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
        const float3 albedo = all_textures[NonUniformResourceIndex(albedo_id)].SampleLevel(float2(0.5), 100.0).rgb * material.base_color_factor;

        radiance = albedo * in_envmap.SampleLevel(hit_normal, 999.0).rgb;
        for(uint k = 0; k != light_count; ++k) {
            const DirectionalLight light = lights[k];
            const float n_l = dot(light.direction, hit_normal);
            if(n_l > 0.0) {
                if(!trace_inline(tlas, hit.position, light.direction, 0.001, 10000.0).hit) {
                    radiance += albedo * light.color * n_l;
                }
            }
        }
    } else {
        radiance = in_envmap.SampleLevel(trace_dir, 0.0).rgb;
    }

    const SH<float3> sh = SH<float>.basis(trace_dir) * radiance;
    atomic_add_sh(index, sh);

#endif

#ifdef APPLY
    const uint index = find_bucket_index(hash_table, key, hash_size);
    if(index == constants.max_uint) {
        return;
    }

    float3 gi = float3(0.0);

    SH<float3> sh;
    if(read_sh(index, sh)) {
        gi = max(0.0, sh.eval_hemisphere_cosine_weighted(normal));
    }

    out_image[coord] = float4(gi, 1.0);
#endif
}
#endif
