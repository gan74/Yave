#ifndef GI_SLANG
#define GI_SLANG

#include "utils.slang"


namespace gi {

static const uint grid_size = 32;
static const uint atlas_size = 256;
static const uint probe_size = 16;

uint compute_probe_index(int3 grid_coord) {
    return (grid_coord.x * grid_size * grid_size) + (grid_coord.y * grid_size) + grid_coord.z;
}

int3 compute_grid_coord(float3 world_pos) {
    return int3(round(world_pos)) % grid_size;
}

int3 compute_grid_coord(float3 world_pos, float3 normal) {
#if 0
    int3 grid;
    for(uint i = 0; i != 3; ++i) {
        grid[i] = int(normal[i] > 0.0 ? ceil(world_pos[i]) : floor(world_pos[i]));
    }
    return grid % grid_size;
#else
    return int3(round(world_pos + normal * 0.5)) % grid_size;
#endif
}


int3 compute_grid_coord(uint probe_index) {
    const int3 coord = int3(
        (probe_index / (grid_size * grid_size)) % grid_size,
        (probe_index / grid_size) % grid_size,
        probe_index % grid_size
    );
    return coord;
}

int3 compute_world_coord(int3 grid_coord, float3 cam_pos) {
    const int3 c = int3(round(cam_pos)) + (grid_size / 2);
    const int3 n = (c - grid_coord) / grid_size;
    return grid_coord + n * grid_size;
}

uint2 compute_atlas_coord(uint probe_index) {
    return uint2(probe_index / atlas_size, probe_index % atlas_size);
}

}


#endif // GI_SLANG
