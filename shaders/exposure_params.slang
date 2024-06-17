import "glsl";

#include "lib/utils.slang"

[[vk::binding(0)]]
RWStructuredBuffer<ExposureParams> exposure_params;

[[vk::binding(1)]]
Sampler2D<uint> in_histogram;

groupshared uint histogram[lum_histogram_size];
groupshared uint total;



[shader("compute")]
[numthreads(lum_histogram_size)]
void comp_main() {
    const uint index = gl_LocalInvocationIndex;
    uint bin_size = in_histogram[int2(index, 0)];

    if(index == 0) {
        total = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(total, bin_size);
    histogram[index] = bin_size;

    GroupMemoryBarrierWithGroupSync();

    // Make this not serial
    if(index == 0) {
        for(uint i = 1; i < lum_histogram_size; ++i) {
            histogram[i] += histogram[i - 1];
        }
    }

    GroupMemoryBarrierWithGroupSync();

    const uint total = uint(total * 0.95); // Kill the top 5% of brightest pixels
    const uint less_eq = histogram[index];
    if(less_eq > total) {
        const uint less_than_bin = less_eq - bin_size;
        bin_size = total - min(total, less_than_bin);
    }

    GroupMemoryBarrierWithGroupSync();

    histogram[index] = bin_size * index;

    GroupMemoryBarrierWithGroupSync();

    // Make this not serial
    if(index == 0) {
        for(uint i = lum_histogram_size - 1; i != 0; --i) {
            histogram[i - 1] += histogram[i];
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if(index == 0) {
        const uint weighted = histogram[0];
        const uint black = bin_size;
        const uint non_black = max(total, black + 1) - black;

        const float log_avg = (weighted / (lum_histogram_mul * non_black)) - lum_histogram_offset;
        const float avg = exp2(log_avg);

        const float EV100 = avg_to_EV100(exposure_params[0].avg_lum);

        ExposureParams params;
        {
            params.exposure = EV100_to_exposure(EV100);
            params.avg_lum = avg;
            params.max_lum = avg * 9.6;
        }
        exposure_params[0] = params;
    }
}

