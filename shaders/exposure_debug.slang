import "glsl";

#include "lib/utils.slang"


static const uint bar_width = 4;
static const uint bar_height = 64;
static const int2 display_offset = int2(8, -8);


[[vk::binding(0)]]
RWTexture2D out_color;

[[vk::binding(1)]]
Sampler2D<uint> in_histogram;

[[vk::binding(2)]]
RWStructuredBuffer<ExposureParams> exposure_params;


groupshared uint total;
groupshared uint max_bin;

void draw_line(float lum, int x_offset, int image_height, float3 color) {
    const float min_lum = histogram_bin_to_lum(LocalInvocationIndex);
    const float max_lum = histogram_bin_to_lum(LocalInvocationIndex + 1);

    if(lum > min_lum && lum < max_lum) {
        const float t = (lum - min_lum) / (max_lum - min_lum);
        const int x = int(round(t * bar_width));
        for(int y = -4; y != bar_height + 4; ++y) {
            out_color[int2(x_offset + x, image_height - y) + display_offset] = float4(color, 1.0);
        }
    }
}

[shader("compute")]
[numthreads(lum_histogram_size)]
void comp_main() {
    const ExposureParams params = exposure_params[0];
    const uint bin_size = in_histogram[int2(LocalInvocationIndex, 0)];

    if(LocalInvocationIndex == 0) {
        total = 0;
        max_bin = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedAdd(total, bin_size);
    InterlockedMax(max_bin, bin_size);

    GroupMemoryBarrierWithGroupSync();

    const int bar = int(max(1, (bin_size * bar_height) / max_bin));
    const int x_offset = int(LocalInvocationIndex * (bar_width + 1));
    const int image_height = int(image_size(out_color).y);

    for(int x = 0; x != bar_width; ++x) {
        for(int y = 0; y != bar; ++y) {
            const float4 color = float4(bin_size > 0 ? 1.0 : 0.25);
            out_color[int2(x_offset + x, image_height - y) + display_offset] = color;
        }
    }

    draw_line(params.avg_lum, x_offset, image_height, float3(0.0, 1.0, 0.0));
    draw_line(params.max_lum, x_offset, image_height, float3(1.0, 0.0, 0.0));
}

