import "glsl";

#include "../lib/utils.slang"


[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2D<float> in_seletion_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_seletion_id;

static const float3 selection_color = float3(241.0, 153.0, 41.0) / 255.0;
static const int size = 2;

[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const int2 coord = int2(semantics.frag_coord.xy);
    const uint base_id = in_seletion_id[coord].r;

    float min_depth = 1.0;
    float alpha = 0.0;
    if(base_id != 0) {
        min_depth = in_seletion_depth[coord].r;
        // alpha = max(0.0, sin(dot(float2(1.0, 1.0), semantics.frag_coord.xy)) * 0.5);
    } else {
        float sum = 0.0;
        float total = 0.0;
        for(int i = -size; i <= size; ++i) {
            for(int j = -size; j <= size; ++j) {
                const uint id = in_seletion_id[coord + int2(i, j)].r;
                const float w = dot(float2(i, j), float2(i, j));
                if(id != base_id) {
                    min_depth = min(min_depth, in_seletion_depth[coord + int2(i, j)].r);
                    sum += w;
                }
                total += w;
            }
        }
        if(sum > 0.0) {
            alpha = saturate(sum / total * 8.0);
        }
    }

    return float4(selection_color, alpha);

    // This is broken by TAA
    /*if(texelFetch(in_depth, coords, 0).r > min_depth) {
        out_color.a *= 0.35;
    }*/
}

