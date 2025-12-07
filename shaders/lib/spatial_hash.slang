#ifndef SPATIAL_HASH_SLANG
#define SPATIAL_HASH_SLANG

#include "utils.slang"
#include "hash.slang"


static const uint max_probe_len = 8;

struct HashKey {
    uint prim;
    uint sec;

    __init() {
        prim = 0;
        sec = 0;
    }

    [mutating]
    private void append(uint val) {
        prim = hash.pcg(prim + val);
        sec = hash.xx32(sec + val);
    }

    static uint lod_level(float view_dist, float lod_dist) {
        return uint(exp2(floor(log2(1.0 + (view_dist / lod_dist)))));
    }

    static HashKey make_key(float3 world_pos, float3 normal, float base_cell_size, uint lod, float3 jitter = float3(0.0)) {
        const float cell_size = base_cell_size * lod;
        const int3 p_i = int3(floor(world_pos / cell_size + jitter));
        const int3 n_i = int3(normal * 3.0);

        HashKey k;
        {
            k.append(n_i.z);
            k.append(n_i.y);
            k.append(n_i.x);
            k.append(p_i.z);
            k.append(p_i.y);
            k.append(p_i.x);
            k.append(lod);
        }
        return k;
    }
};


uint find_update_bucket_index(RWStructuredBuffer<uint> hash_table, HashKey key, uint hash_size, out bool inserted) {
    const uint check = max(1, key.sec);
    const uint hash_mask = hash_size - 1;

    for(uint i = 0; i != max_probe_len; ++i) {
        const uint index = (key.prim + i) & hash_mask;

        uint prev = 0;
        InterlockedCompareExchange(hash_table[index], 0, check, prev);

        inserted = (prev == 0);
        if(prev == 0 || prev == check) {
            return index;
        }
    }

    return constants.max_uint;
}

uint find_bucket_index(RWStructuredBuffer<uint> hash_table, HashKey key, uint hash_size) {
    const uint check = max(1, key.sec);
    const uint hash_mask = hash_size - 1;

    for(uint i = 0; i != max_probe_len; ++i) {
        const uint index = (key.prim + i) & hash_mask;

        const uint prev = hash_table[index];

        if(prev == check) {
            return index;
        }
    }

    return constants.max_uint;
}

#endif // SPATIAL_HASH_SLANG
