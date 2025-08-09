#include "../lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
RWTexture2D<float4> out_color;

[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const int2 coord = int2(semantics.global_id.xy);

    if(is_depth_far(in_depth[coord])) {
        float4 color = float4(0.0);
        for(int x = -1; x <= 1; ++x) {
            for(int y = -1; y <= 1; ++y) {
                if(!is_depth_far(in_depth[coord + int2(x, y)])) {
                    color += float4(in_color[coord + int2(x, y)].rgb, 1.0);
                }
            }
        }
        out_color[coord] = color.a > 0.0 ? float4(color.rgb / color.a, color.a / 9.0) : float4(0.0); 
    } else {
        const float4 color = in_color[coord];
        out_color[coord] = float4(color.rgb, 1.0);
    }

}

