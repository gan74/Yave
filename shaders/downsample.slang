#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;


[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const float4 samples[4] = {
        in_color.GatherRed(in_uv),
        in_color.GatherGreen(in_uv),
        in_color.GatherBlue(in_uv),
        in_color.GatherAlpha(in_uv),
    };

    float4 avg;
    for(uint i = 0; i != 4; ++i) {
        avg[i] = dot(samples[i], 0.25);
    }


    float4 best_smp;
    float best_score = constants.max_float;
    for(uint i = 0; i != 4; ++i) {
        const float4 smp = float4(samples[0][i], samples[1][i], samples[2][i], samples[3][i]);
        const float4 diff = (smp - avg);
        const float score = dot(diff, diff);
        if(score < best_score) {
            best_score = score;
            best_smp = smp;
        }
    }

    return best_smp;
}

