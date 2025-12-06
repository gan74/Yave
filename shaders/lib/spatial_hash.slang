#ifndef SPATIAL_HASH_SLANG
#define SPATIAL_HASH_SLANG

#include "utils.slang"
#include "hash.slang"

struct HashKey {
    uint prim;
    uint sec;

    __init() {
        prim = 0;
        sec = 0;
    }

    [mutating]
    void append(uint val) {
        prim = hash.pcg(prim + val);
        sec = hash.xx32(sec + val);
    }

    static HashKey make_key(float3 world_pos, float3 normal, float view_dist, float base_cell_size, float lod_dist) {
        const uint lod = uint(exp2(floor(log2(1.0 + (view_dist / lod_dist)))));
        const float cell_size = base_cell_size * lod;
        const int3 p_i = int3(floor(world_pos / cell_size));
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

uint find_bucket_index<let update : bool>(RWStructuredBuffer<uint> hash_table, HashKey key, uint hash_size) {
    const uint check = max(1, key.sec);
    const uint hash_mask = hash_size - 1;

    for(uint i = 0; i != 8; ++i) {
        const uint index = (key.prim + i) & hash_mask;

        uint prev = 0;
        if(update) {
            InterlockedCompareExchange(hash_table[index], 0, check, prev);
        } else {
            prev = hash_table[index];
        }

        if(prev == 0 || prev == check) {
            return index;
        }
    }

    return constants.max_uint;
}

#endif // SPATIAL_HASH_SLANG
