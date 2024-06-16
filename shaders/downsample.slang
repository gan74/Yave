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

    uint index = 0;
    float best = abs(dot(samples[0], avg));
    for(uint i = 1; i != 4; ++i) {
        const float score = abs(dot(samples[i], avg));
        if(score < best) {
            best = score;
            index = i;
        }
    }

    return samples[index];
}

