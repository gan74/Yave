#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/hash.slang"
#include "lib/spatial_hash.slang"


// -------------------------------- DEFINES --------------------------------

#define SAMPLE_BUFFER_SIZE 256

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
RWTexture2D<float> out_ao;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
cbuffer SampleDirData {
    float4 sample_dirs[SAMPLE_BUFFER_SIZE];
};

[[vk::binding(5)]]
cbuffer Params {
    uint sample_count;
    float max_dist;
    float lod_dist;
    float base_cell_size;

    uint hash_size;
    uint sample_clamp;
    uint frame_id;
    uint reset_hash;
};

[[vk::binding(6)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(7)]]
RWStructuredBuffer<uint> hash_table;


float3 compute_sample_dir(float3 normal, uint i) {
    const float3 s = sample_dirs[i % SAMPLE_BUFFER_SIZE].xyz;
    return dot(normal, s) < 0.0 ? -s : s;
}


uint payload_index(uint h) {
    return hash_size + h;
}

uint timestamp_index(uint h) {
    return hash_size * 2 + h;
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
    const float3 normal = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[coord])).normal;
    const float view_dist = length(world_pos - camera.position);

    const HashKey key = HashKey.make_key(world_pos, normal, view_dist, base_cell_size, lod_dist);

#ifdef UPDATE
    const uint hash_index = find_bucket_index<true>(hash_table, key, hash_size);
    if(hash_index != constants.max_uint) {
        const float min_dist = lerp(0.001, 0.01, view_dist / (view_dist + 10.0));
        const uint sample_index_offset = hash.xorshift(semantics.global_id.x + hash.xorshift(semantics.global_id.y));

        uint occ = 0;
        for(uint i = 0; i != sample_count; ++i) {
            const float3 sample_dir = compute_sample_dir(normal, i * 7 + sample_index_offset);
            if(!trace_inline(tlas, world_pos, sample_dir, min_dist, max_dist).hit) {
                ++occ;
            }
        }
        const uint payload = (occ << 16) + sample_count;

        uint prev;
        InterlockedAdd(hash_table[payload_index(hash_index)], payload, prev);
        hash_table[timestamp_index(hash_index)] = frame_id;
    }
#else
    const uint hash_index = find_bucket_index<false>(hash_table, key, hash_size);
    if(hash_index != constants.max_uint) {
        const uint payload = hash_table[payload_index(hash_index)];
        const float ao = float(payload >> 16) / float(payload & 0xFFFF);
        out_ao[coord] = ao;
    } else {
        out_ao[coord] = 0.0;
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
        hash_table[payload_index(h)] = 0;
    } else {
        const uint payload = hash_table[payload_index(h)];
        const uint occ = payload >> 16;
        const uint samples = payload & 0xFFFF;
        if(samples > sample_clamp) {
            const float ao = saturate(float(occ) / float(samples));
            const uint new_payload = (uint(ao * sample_clamp) << 16) | sample_clamp;
            hash_table[payload_index(h)] = new_payload;
        }
    }
}
#endif

