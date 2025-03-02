#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
cbuffer Settings_Inline {
    float sigma;
    float k_sigma;
    float threshold;
};

// Base on glslSmartDeNoise by Michele Morrone
// https://github.com/BrutPitt/glslSmartDeNoise/tree/master

float4 smart_denoise(Sampler2D tex, float2 uv, float sigma, float k_sigma, float threshold) {
    const float radius = round(k_sigma * sigma);
    const float rad_sq = radius * radius;

    float inv_2_sigma_sq = 0.5 / (sigma * sigma);
    float inv_2pi_sigma_sq = inv_pi * inv_2_sigma_sq;

    float inv_2_threshold_sq = 0.5 / (threshold * threshold);
    float inv_2pi_threshold_sq = inv_sqrt_2pi / threshold;

    const float4 center = tex.Sample(uv);


    const float2 size = float2(image_size(tex).xy);

    float2 d;
    float total = 0.0;
    float4 acc = float4(0.0);
    for(d.x = -radius; d.x <= radius; d.x += 1.0) {
        const float pt = sqrt(rad_sq - d.x * d.x);       // pt = yRadius: have circular trend
        for(d.y = -pt; d.y <= pt; d.y += 1.0) {
            const float blur_factor = exp(-dot(d, d) * inv_2_sigma_sq) * inv_2pi_sigma_sq;

            const float4 px =  tex.Sample(uv + d / size);
            const float4 diff = px - center;
            const float delta = exp(-dot(diff.rgb, diff.rgb) * inv_2_threshold_sq) * inv_2pi_threshold_sq * blur_factor;

            total += delta;
            acc += delta * px;
        }
    }
    return acc / total;
}

[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    return smart_denoise(in_color, in_uv, sigma, k_sigma, threshold);

}
