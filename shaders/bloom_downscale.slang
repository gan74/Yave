#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
ConstantBuffer<ExposureParams> exposure_params;

[[vk::binding(2)]]
cbuffer BloomParams_Inline {
    uint pass;
};

float3 karis(float3 hdr) {
    const float avg = 1.0f / (1.0f + (luminance(hdr) * 0.25));
    return hdr * avg;
}

float3 max_bright(float3 a, float3 b) {
    return luminance(a) > luminance(b) ? a : b;
}


[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const float2 texel_size = 1.0 / image_size(in_color);

    const float3 a = in_color.Sample(in_uv + float2(-2.0,  2.0) * texel_size).rgb;
    const float3 b = in_color.Sample(in_uv + float2( 0.0,  2.0) * texel_size).rgb;
    const float3 c = in_color.Sample(in_uv + float2( 2.0,  2.0) * texel_size).rgb;
    const float3 d = in_color.Sample(in_uv + float2(-2.0,  0.0) * texel_size).rgb;
    const float3 e = in_color.Sample(in_uv).rgb;
    const float3 f = in_color.Sample(in_uv + float2( 2.0,  0.0) * texel_size).rgb;
    const float3 g = in_color.Sample(in_uv + float2(-2.0, -2.0) * texel_size).rgb;
    const float3 h = in_color.Sample(in_uv + float2( 0.0, -2.0) * texel_size).rgb;
    const float3 i = in_color.Sample(in_uv + float2( 2.0, -2.0) * texel_size).rgb;
    const float3 j = in_color.Sample(in_uv + float2(-1.0,  1.0) * texel_size).rgb;
    const float3 k = in_color.Sample(in_uv + float2( 1.0,  1.0) * texel_size).rgb;
    const float3 l = in_color.Sample(in_uv + float2(-1.0, -1.0) * texel_size).rgb;
    const float3 m = in_color.Sample(in_uv + float2( 1.0, -1.0) * texel_size).rgb;

    if(pass == 0) {
        const float3 g0 = (a + b + d + e) * 0.125 * 0.25;
        const float3 g1 = (b + c + e + f) * 0.125 * 0.25;
        const float3 g2 = (d + e + g + h) * 0.125 * 0.25;
        const float3 g3 = (e + f + h + i) * 0.125 * 0.25;
        const float3 g4 = (j + k + l + m) * 0.5 * 0.25;
        const float3 filtered = karis(g0) + karis(g1) + karis(g2) + karis(g3) + karis(g4);
        return float4(max(float3(0.0), filtered - float3(exposure_params.max_lum)), 1.0);
    }

    return float4(
        (e * 0.125) +
        (a + c + g + i) * 0.03125 +
        (b + d + f + h) * 0.0625 +
        (j + k + l + m) * 0.125,
    1.0);
}

