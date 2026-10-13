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
    private void append(uint val) {
        prim = hash.pcg(prim + val);
        sec = hash.xx32(sec + val);
    }

    static float float_lod_level(float view_dist, float lod_dist) {
        return log2(1.0 + (view_dist / lod_dist));
    }

    static uint lod_level(float view_dist, float lod_dist) {
        return uint(float_lod_level(view_dist, lod_dist));
    }

    static HashKey make_key(float3 world_pos, float3 normal, float base_cell_size, uint lod, float3 pos_jitter = float3(0.0), float3 norm_jitter = float3(0.0)) {
        const float cell_size = base_cell_size * exp2(float(lod));
        const int3 p_i = int3(floor(world_pos / cell_size + pos_jitter));
        const int3 n_i = int3(floor(normal * 3.0 + norm_jitter));

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

    static HashKey make_key(float3 world_pos, float base_cell_size, uint lod, float3 pos_jitter = float3(0.0)) {
        const float cell_size = base_cell_size * exp2(float(lod));
        const int3 p_i = int3(floor(world_pos / cell_size + pos_jitter));

        HashKey k;
        {
            k.append(p_i.z);
            k.append(p_i.y);
            k.append(p_i.x);
            k.append(lod);
        }
        return k;
    }
};



bool operator!=(HashKey a, HashKey b) {
    return a.prim != b.prim || a.sec != b.sec;
}





static const uint max_probe_len = 8;
static const uint hash_empty = 0;
static const uint hash_tombstone = 1;

uint bucket_check(HashKey key) {
    return max(hash_tombstone + 1, key.sec);
}

void evict_bucket_unsynchronized(RWStructuredBuffer<uint> hash_table, uint index, bool reset) {
    hash_table[index] = (reset || hash_table[index] == hash_empty) ? hash_empty : hash_tombstone;
}

uint find_update_bucket_index(RWStructuredBuffer<uint> hash_table, HashKey key, uint hash_size, out bool inserted) {
    const uint check = bucket_check(key);
    const uint hash_mask = hash_size - 1;

    inserted = false;

    uint free_index = constants.max_uint;
    uint free_value = hash_empty;
    for(uint i = 0; i != max_probe_len; ++i) {
        const uint index = (key.prim + i) & hash_mask;
        const uint prev = hash_table[index];
        if(prev == check) {
            return index;
        }
        if(prev <= hash_tombstone && free_index == constants.max_uint) {
            free_index = index;
            free_value = prev;
        }
        if(prev == hash_empty) {
            break;
        }
    }

    if(free_index == constants.max_uint) {
        return constants.max_uint;
    }

    uint prev = 0;
    InterlockedCompareExchange(hash_table[free_index], free_value, check, prev);
    inserted = (prev == free_value);
    return (inserted || prev == check) ? free_index : constants.max_uint;
}

uint find_bucket_index(RWStructuredBuffer<uint> hash_table, HashKey key, uint hash_size) {
    const uint check = bucket_check(key);
    const uint hash_mask = hash_size - 1;

    for(uint i = 0; i != max_probe_len; ++i) {
        const uint index = (key.prim + i) & hash_mask;

        const uint prev = hash_table[index];

        if(prev == check) {
            return index;
        }
        if(prev == hash_empty) {
            break;
        }
    }

    return constants.max_uint;
}

#endif // SPATIAL_HASH_SLANG
