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
    if(!ddgi.in_range(world_pos, probe_spacing)) {
        return;
    }

    const int3 grid_coord = ddgi.compute_grid_coord(world_pos, probe_spacing);
    const uint2 probe_grid_texel = ddgi.probe_grid_texel(grid_coord);

    const uint locked_index = ddgi.invalid_probe_index - 1;

    uint prev;
    InterlockedCompareExchange(probe_grid[probe_grid_texel], ddgi.invalid_probe_index, locked_index, prev);
    if(prev == ddgi.invalid_probe_index) {
        uint index;
        InterlockedAdd(probe_counter[0], 1, index);
        probe_grid[probe_grid_texel] = index < max_probe_count ? index : ddgi.invalid_probe_index;
    }
}

#endif
