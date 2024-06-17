import "glsl";

#include "lib/utils.slang"


// https://bruop.github.io/exposure/
static const uint thread_count = 64;
static const uint bin_per_thread = lum_histogram_size / thread_count;


[[vk::binding(0)]]
RWTexture2D<uint> out_histogram;

[[vk::binding(1)]]
Sampler2D in_color;


groupshared uint histogram[lum_histogram_size];


uint compute_index(uint i) {
    return gl_LocalInvocationIndex * bin_per_thread + i;
}

[shader("compute")]
[numthreads(thread_count)]
void comp_main() {
    const uint2 base_coord = uint2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y * thread_count);
    const uint2 size = image_size(in_color);

    for(uint i = 0; i != bin_per_thread; ++i) {
        histogram[compute_index(i)] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    if(base_coord.x < size.x) {
        for(uint i = 0; i != thread_count; ++i) {
            const int2 coord = int2(base_coord.x, base_coord.y + i);
            if(coord.y < size.y) {
                const float lum = luminance(in_color[coord].rgb);
                const uint bin = lum_to_histogram_bin(lum);
                InterlockedAdd(histogram[bin], 1);
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    for(uint i = 0; i != bin_per_thread; ++i) {
        const uint index = compute_index(i);
        if(histogram[index] != 0) {
            InterlockedAdd(out_histogram[int2(index, 0)], histogram[index]);
        }
    }
}

