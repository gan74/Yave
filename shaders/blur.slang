
#include "lib/utils.slang"

#if defined(HORIZONTAL)
#define BLUR_DIRECTION x
#endif
#if defined(VERTICAL)
 #define BLUR_DIRECTION y
#endif


[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
cbuffer Weights_Inline {
    float4 weights[2];
};


[shader("fragment")]
float4 blur(Sampler2D tex, float2 uv) {
    const float2 inv_scale = 1.0 / image_size(tex).xy;
    float4 total = tex.Sample(uv) * weights[0][0];
    for(int i = 1; i != 8; ++i) {

        float2 offset = float2(0.0, 0.0);
        offset.BLUR_DIRECTION = (inv_scale.BLUR_DIRECTION * i);
        const float w = weights[i / 4][i % 4];
        total += tex.Sample(uv + offset) * w;
        total += tex.Sample(uv - offset) * w;
    }

    return total;
}

float4 frag_main(float2 in_uv) : SV_Target0 {
    return blur(in_color, in_uv);
}

