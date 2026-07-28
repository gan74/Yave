#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/ddgi.slang"


[[vk::binding(0)]]
RWTexture2D<uint> probe_grid;

[[vk::binding(1)]]
RWStructuredBuffer<uint> probe_counter;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D<uint> in_rt1;

[[vk::binding(4)]]
ConstantBuffer<Camera> camera;

[[vk::binding(5)]]
cbuffer Params {
    float probe_spacing;
    uint max_probe_count;
};



#ifdef CLEAR

[shader("compute")]
[numthreads(8, 8)]
void main() {
    if(semantics.global_index == 0) {
        probe_counter[0] = 0;
    }

    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(probe_grid);
    if(any(coord >= size)) {
        return;
    }
    probe_grid[coord] = ddgi.invalid_probe_index;
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
                uint index;
                InterlockedAdd(probe_counter[0], 1, index);
                probe_grid[probe_grid_texel] = index < max_probe_count ? index : ddgi.invalid_probe_index;
            }
        }
    }
}

#endif
