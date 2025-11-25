#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"


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
cbuffer Params_Inline {
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

uint pcg(uint v) {
    const uint state = v * 747796405u + 2891336453u;
    const uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint xxhash32(uint p) {
    const uint PRIME32_2 = 2246822519U, PRIME32_3 = 3266489917U;
    const uint PRIME32_4 = 668265263U, PRIME32_5 = 374761393U;
    uint h32 = p + PRIME32_5;
    h32 = PRIME32_4 * ((h32 << 17) | (h32 >> (32 - 17)));
    h32 = PRIME32_2 * (h32 ^ (h32 >> 15));
    h32 = PRIME32_3 * (h32 ^ (h32 >> 13));
    return h32 ^ (h32 >> 16);
}

void hash_append(inout uint2 h, uint v) {
    h = uint2(
        pcg(h.x + v),
        xxhash32(h.y + v),
    );
}



uint checksum_index(uint h) {
    return h;
}

uint payload_index(uint h) {
    return hash_size + h;
}

uint timestamp_index(uint h) {
    return hash_size * 2 + h;
}


uint find_bucket(uint2 h) {
    const uint check = max(1, h.y);
    const uint hash_mask = hash_size - 1;

    for(uint i = 0; i != 8; ++i) {
        const uint index = (h.x + i) & hash_mask;

        uint prev = 0;
        InterlockedCompareExchange(hash_table[checksum_index(index)], 0, check, prev);

        if(prev == 0 || prev == check) {
            return index;
        }
    }

    return constants.max_uint;
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

    const uint lod = uint(exp2(floor(log2(1.0 + (view_dist  / lod_dist)))));
    const float cell_size = base_cell_size * lod;
    const int3 p_i = int3(floor(world_pos / cell_size));
    const int3 n_i = int3(normal * 3.0);

    uint2 h = uint2(0, 0);
    {
        hash_append(h, n_i.z);
        hash_append(h, n_i.y);
        hash_append(h, n_i.x);

        hash_append(h, p_i.z);
        hash_append(h, p_i.y);
        hash_append(h, p_i.x);

        hash_append(h, lod);
    }

    const uint hash_index = find_bucket(h);
    const bool ok = hash_index != constants.max_uint;

#ifdef UPDATE
    if(ok) {
        const float min_dist = lerp(0.001, 0.01, view_dist / (view_dist + 10.0));
        const uint sample_index_offset = hash(semantics.global_id.xy);

        uint hits = 0;
        for(uint i = 0; i != sample_count; ++i) {
            const float3 sample_dir = compute_sample_dir(normal, i * 7 + sample_index_offset);
            if(trace_first_hit(tlas, world_pos, sample_dir, min_dist, max_dist) >= max_dist) {
                ++hits;
            }
        }
        const uint payload = (hits << 16) + sample_count;

        uint prev;
        InterlockedAdd(hash_table[payload_index(hash_index)], payload, prev);
        hash_table[timestamp_index(hash_index)] = frame_id;
    }
#else
    if(ok) {
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
        hash_table[checksum_index(h)] = 0;
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

