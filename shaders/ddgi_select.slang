#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/structs.slang"
#include "lib/ddgi.slang"
#include "lib/spatial_hash.slang"
#include "lib/rng.slang"


[[vk::binding(0)]]
RWStructuredBuffer<uint> hash_table;

[[vk::binding(1)]]
RWStructuredBuffer<uint> active_probes;

[[vk::binding(2)]]
RWStructuredBuffer<DDGIProbeData> probe_datas;

[[vk::binding(3)]]
Sampler2D<float> in_depth;

[[vk::binding(4)]]
Sampler2D<uint> in_rt1;

[[vk::binding(5)]]
ConstantBuffer<Camera> camera;

[[vk::binding(6)]]
cbuffer Params {
    float probe_spacing;
    uint max_probe_count;
    uint frame_id;
    uint max_visible_age;

    uint hash_size;
    uint padding_0;
    uint padding_1;
    uint padding_2;
};



void add_active(uint probe_index) {
    uint index;
    InterlockedAdd(active_probes[0], 1, index);
    if(index < max_probe_count) {
        active_probes[2 + index] = probe_index;
    }
}

void add_inactive(uint probe_index) {
    uint index;
    InterlockedAdd(active_probes[1], 1, index);
    if(index < max_probe_count) {
        active_probes[max_probe_count + 1 - index] = probe_index;
    }
}

void clear_hash_bucket(uint bucket) {
    hash_table[bucket] = 0;
    hash_table[ddgi.payload_index(bucket, hash_size)] = constants.max_uint;
}



#ifdef CLEAR

[shader("compute")]
[numthreads(64)]
void main() {
    if(semantics.global_index == 0) {
        active_probes[0] = 0;
        active_probes[1] = 0;
    }

    if(semantics.global_index < hash_size) {
        clear_hash_bucket(semantics.global_index);
    }

    if(semantics.global_index < max_probe_count) {
        probe_datas[semantics.global_index].hash_bucket = constants.max_uint;
    }
}

#elif defined(TRIM_CLEAR)

[shader("compute")]
[numthreads(1)]
void main() {
    active_probes[0] = 0;
    active_probes[1] = 0;
}

#elif defined(TRIM)

[shader("compute")]
[numthreads(64)]
void main() {
    const uint probe_index = semantics.global_index;
    if(probe_index >= max_probe_count) {
        return;
    }

    const uint hash_bucket = probe_datas[probe_index].hash_bucket;
    if(hash_bucket == constants.max_uint) {
        add_inactive(probe_index);
    } else if(frame_id - probe_datas[probe_index].last_visible > max_visible_age) {
        clear_hash_bucket(hash_bucket);
        probe_datas[probe_index].hash_bucket = constants.max_uint;
        add_inactive(probe_index);
    } else {
        add_active(probe_index);
    }
}

#else

uint find_first(uint4 mask) {
    if(mask.x != 0) {
        return firstbitlow(mask.x);
    }
    if(mask.y != 0) {
        return 32 + firstbitlow(mask.y);
    }
    if(mask.z != 0) {
        return 64 + firstbitlow(mask.z);
    }
    return 96 + firstbitlow(mask.w);
}

uint allocate_probe_index() {
    uint inactive_index;
    InterlockedAdd(active_probes[1], -1, inactive_index);

    if(inactive_index >= max_probe_count) {
        return constants.max_uint;
    }

    const uint index = active_probes[2 + inactive_index];

    uint active_index;
    InterlockedAdd(active_probes[0], 1, active_index);
    active_probes[2 + active_index] = index;

    return index;
}


[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(in_depth).xy;
    if(any(coord >= size)) {
        return;
    }

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        return;
    }

    const float2 uv = (float2(coord) + 0.5) / float2(size);
    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const float3 normal = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[coord])).normal;

    const DDGISamplePos sample_pos = DDGISamplePos.make_pos(world_pos, normal, normalize(camera.position - world_pos), probe_spacing);

    for(uint i = 0; i != 8; ++i) {
        const int3 probe_coord = sample_pos.corner(i);

        if(any(abs(probe_coord) > 32)) {
            continue;
        }

        bool inserted = false;
        const uint hash_bucket = find_update_bucket_index(hash_table, HashKey.make_key(probe_coord), hash_size, inserted);

        if(hash_bucket == constants.max_uint) {
            continue;
        }

        const uint payload_index = ddgi.payload_index(hash_bucket, hash_size);
        if(!inserted) {
            const uint index = hash_table[payload_index];
            if(index < max_probe_count && probe_datas[index].hash_bucket == hash_bucket) {
                probe_datas[index].last_visible = frame_id;
            }
            continue;
        }

        const uint index = allocate_probe_index();
        if(index >= max_probe_count) {
            clear_hash_bucket(hash_bucket);
            continue;
        }

        hash_table[payload_index] = index;
        probe_datas[index].hash_bucket = hash_bucket;
        probe_datas[index].probe_coord = probe_coord;
        probe_datas[index].last_visible = frame_id;
        probe_datas[index].allocated_frame = frame_id;
    }
}

#endif
