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

void read_sh(uint h, out SH r, out SH g, out SH b, out float weight) {
    const uint base = sh_offset(h);
    r = SH(
        sh_table[base + 0], sh_table[base + 1], sh_table[base + 2],
        sh_table[base + 3], sh_table[base + 4], sh_table[base + 5],
        sh_table[base + 6], sh_table[base + 7], sh_table[base + 8]
    );
    g = SH(
        sh_table[base + 9], sh_table[base + 10], sh_table[base + 11],
        sh_table[base + 12], sh_table[base + 13], sh_table[base + 14],
        sh_table[base + 15], sh_table[base + 16], sh_table[base + 17]
    );
    b = SH(
        sh_table[base + 18], sh_table[base + 19], sh_table[base + 20],
        sh_table[base + 21], sh_table[base + 22], sh_table[base + 23],
        sh_table[base + 24], sh_table[base + 25], sh_table[base + 26]
    );
    weight = sh_table[base + 27];
}

void atomic_add_sh(uint h, SH r, SH g, SH b) {
    const uint base = sh_offset(h);
    float prev;

    InterlockedAdd(sh_table[base +  0], r.Y00, prev);
    InterlockedAdd(sh_table[base +  1], r.Y11, prev);
    InterlockedAdd(sh_table[base +  2], r.Y10, prev);
    InterlockedAdd(sh_table[base +  3], r.Y1_1, prev);
    InterlockedAdd(sh_table[base +  4], r.Y21, prev);
    InterlockedAdd(sh_table[base +  5], r.Y2_1, prev);
    InterlockedAdd(sh_table[base +  6], r.Y2_2, prev);
    InterlockedAdd(sh_table[base +  7], r.Y20, prev);
    InterlockedAdd(sh_table[base +  8], r.Y22, prev);

    InterlockedAdd(sh_table[base +  9], g.Y00, prev);
    InterlockedAdd(sh_table[base + 10], g.Y11, prev);
    InterlockedAdd(sh_table[base + 11], g.Y10, prev);
    InterlockedAdd(sh_table[base + 12], g.Y1_1, prev);
    InterlockedAdd(sh_table[base + 13], g.Y21, prev);
    InterlockedAdd(sh_table[base + 14], g.Y2_1, prev);
    InterlockedAdd(sh_table[base + 15], g.Y2_2, prev);
    InterlockedAdd(sh_table[base + 16], g.Y20, prev);
    InterlockedAdd(sh_table[base + 17], g.Y22, prev);

    InterlockedAdd(sh_table[base + 18], b.Y00, prev);
    InterlockedAdd(sh_table[base + 19], b.Y11, prev);
    InterlockedAdd(sh_table[base + 20], b.Y10, prev);
    InterlockedAdd(sh_table[base + 21], b.Y1_1, prev);
    InterlockedAdd(sh_table[base + 22], b.Y21, prev);
    InterlockedAdd(sh_table[base + 23], b.Y2_1, prev);
    InterlockedAdd(sh_table[base + 24], b.Y2_2, prev);
    InterlockedAdd(sh_table[base + 25], b.Y20, prev);
    InterlockedAdd(sh_table[base + 26], b.Y22, prev);

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

    const SH basis = SH.basis(trace_dir);
    const SH r_sh = basis * radiance.r;
    const SH g_sh = basis * radiance.g;
    const SH b_sh = basis * radiance.b;

    /*if(isnan(r_sh) || isnan(g_sh) || isnan(b_sh)) {
        // oof
    } else*/ {
        atomic_add_sh(index, r_sh, g_sh, b_sh);
    }

#endif

#ifdef APPLY
    const uint index = find_bucket_index(hash_table, key, hash_size);
    if(index == constants.max_uint) {
        return;
    }

    SH r_sh, g_sh, b_sh;
    float weight;
    read_sh(index, r_sh, g_sh, b_sh, weight);

    if(weight < 1.0) {
        return;
    }

    const float inv_weight = 1.0 / weight;
    r_sh = r_sh * inv_weight;
    g_sh = g_sh * inv_weight;
    b_sh = b_sh * inv_weight;

    const float3 gi = float3(
        max(0.0, r_sh.eval_hemisphere_cosine_weighted(normal)),
        max(0.0, g_sh.eval_hemisphere_cosine_weighted(normal)),
        max(0.0, b_sh.eval_hemisphere_cosine_weighted(normal))
    );

    out_image[coord] = float4(gi, 1.0);
#endif
}
#endif
