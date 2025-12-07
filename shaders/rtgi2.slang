#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/spatial_hash.slang"


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
    uint padding1;
    uint padding2;
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


float4 eval_material_color(uint instance_id) {
    const MaterialData material = materials[instance_id];
    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
    return all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);
}

uint timestamp_index(uint h) {
    return hash_size + h;
}



#ifndef TRIM
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
    const SurfaceInfo surface = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[coord]));
    const float3 view_vec = (world_pos - camera.position);
    const float view_dist = length(view_vec);

    const HashKey key = HashKey.make_key(world_pos, surface.normal, view_dist, base_cell_size, lod_dist);

#ifdef APPLY
    const uint hash_index = find_bucket_index(hash_table, key, hash_size);
    if(hash_index == constants.max_uint) {
        out_image[coord] = float4(0.0);
    } else {
        float4 gi = float4(0.0);
        for(uint i = 0; i != 4; ++i) {
            gi[i] = sum_table[hash_index * 4 + i];
        }
        out_image[coord] = float4(gi.rgb / gi.a, 1.0);
    }
#else
    bool inserted = false;
    const uint hash_index = find_update_bucket_index(hash_table, key, hash_size, inserted);
    if(hash_index == constants.max_uint) {
        return;
    }

    float4 acc = float4(0.0);

    const uint sample_count = 1;
    const uint sample_index_offset = hash.xorshift(semantics.global_index + hash.xorshift(frame_id));
    for(uint k = 0; k != sample_count; ++k) {
        const float3 trace_dir = create_sample_dir(surface.normal, sample_index_offset * sample_count + k);

        const float tmin = 0.001;
        const float tmax = 10000.0;

        const HitInfo hit = trace_inline(tlas, world_pos, trace_dir, tmin, tmax);

        if(hit.hit) {
            const DirectionalLight light = lights[0];
            const float3 hit_normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
            const float n_l = dot(light.direction, hit_normal);
            if(n_l > 0.0) {
                if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
                    const float4 albedo = eval_material_color(hit.instance_id);
                    acc += float4(albedo.rgb * light.color * n_l * dot(surface.normal, trace_dir), 1.0);
                }
            }
        } else {
            acc += float4(in_envmap.SampleLevel(trace_dir, 0.0).rgb, 1.0);
        }
    }

    hash_table[timestamp_index(hash_index)] = frame_id;

    if(acc.a > 0.0) {
        for(uint i = 0; i != 4; ++i) {
            float prev;
            InterlockedAdd(sum_table[hash_index * 4 + i], acc[i], prev);
        }
    }
#endif
}
#else
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
        float4 acc;
        for(uint i = 0; i != 4; ++i) {
            acc[i] = sum_table[h * 4 + i];
        }

        if(acc.a > max_samples) {
            acc.rgb *= max_samples / acc.a;
            acc.a = max_samples;

            for(uint i = 0; i != 4; ++i) {
                sum_table[h * 4 + i] = acc[i];
            }
        }
    }
}
#endif
