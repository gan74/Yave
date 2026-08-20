#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/structs.slang"
#include "lib/ddgi.slang"


[[vk::binding(0)]]
RWTexture2D<uint> probe_grid;

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
};



void add_active(uint atlas_index) {
    uint index;
    InterlockedAdd(active_probes[0], 1, index);
    if(index < max_probe_count) {
        active_probes[2 + index] = atlas_index;
    }
}

void add_inactive(uint atlas_index) {
    uint index;
    InterlockedAdd(active_probes[1], 1, index);
    if(index < max_probe_count) {
        active_probes[max_probe_count + 1 - index] = atlas_index;
    }
}



#ifdef CLEAR

[shader("compute")]
[numthreads(8, 8)]
void main() {
    if(semantics.global_index == 0) {
        active_probes[0] = 0;
        active_probes[1] = 0;
    }

    if(semantics.global_index < max_probe_count) {
        probe_datas[semantics.global_index].last_visible = 0;
        probe_datas[semantics.global_index].probe_index = ddgi.invalid_probe_index;
    }

    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(probe_grid);
    if(any(coord >= size)) {
        return;
    }
    probe_grid[coord] = ddgi.invalid_probe_index;
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
    const uint atlas_index = semantics.global_index;
    if(atlas_index >= max_probe_count) {
        return;
    }

    const uint grid_probe_index = probe_datas[atlas_index].probe_index;
    if(grid_probe_index == ddgi.invalid_probe_index) {
        add_inactive(atlas_index);
    } else if(frame_id - probe_datas[atlas_index].last_visible > max_visible_age) {
        probe_grid[ddgi.probe_grid_texel(grid_probe_index)] = ddgi.invalid_probe_index;
        probe_datas[atlas_index].probe_index = ddgi.invalid_probe_index;
        add_inactive(atlas_index);
    } else {
        add_active(atlas_index);
    }
}

#else

uint compute_probe_index(int3 coord) {
    return uint(
        coord.x * ddgi.grid_size * ddgi.grid_size +
        coord.y * ddgi.grid_size +
        coord.z
    );
}

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

    if(!ddgi.in_range(world_pos, probe_spacing)) {
        return;
    }

    const DDGISamplePos sample_pos = DDGISamplePos.make_pos(world_pos, normal, normalize(camera.position - world_pos), probe_spacing);

    for(uint i = 0; i != 8; ++i) {
        const int3 grid_coord = sample_pos.corner(i);

        /*const float3 probe_pos = ddgi.probe_world_position(grid_coord, probe_spacing);
        if(dot(probe_pos - sample_pos.biased_world_pos, normal) < 1.0) {
            continue;
        }*/

        const uint probe_index = compute_probe_index(grid_coord);

        const uint leader = find_first(WaveMatch(probe_index));
        if(WaveGetLaneIndex() == leader) {
            const uint locked_index = ddgi.invalid_probe_index - 1;
            const uint2 probe_grid_texel = ddgi.probe_grid_texel(grid_coord);

            uint prev;
            InterlockedCompareExchange(probe_grid[probe_grid_texel], ddgi.invalid_probe_index, locked_index, prev);

            if(prev == ddgi.invalid_probe_index) {
                uint inactive_index;
                InterlockedAdd(active_probes[1], -1, inactive_index);

                if(inactive_index < max_probe_count) {
                    const uint index = active_probes[2 + inactive_index];

                    uint active_index;
                    InterlockedAdd(active_probes[0], 1, active_index);
                    active_probes[2 + active_index] = index;

                    probe_grid[probe_grid_texel] = index;

                    probe_datas[index].last_visible = frame_id;
                    probe_datas[index].probe_index = probe_index;
                    probe_datas[index].allocated_frame = frame_id;
                } else {
                    probe_grid[probe_grid_texel] = ddgi.invalid_probe_index;
                }
                
            } else if(prev != locked_index) {
                probe_datas[prev].last_visible = frame_id;
            }
        }
    }
}

#endif
