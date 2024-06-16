[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
cbuffer BloomParams_Inline {
    float2 filter_size;
    float alpha;
};


[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const float x = filter_size.x;
    const float y = filter_size.y;

    const float3 a = in_color.Sample(in_uv + float2( -x,   y)).rgb;
    const float3 b = in_color.Sample(in_uv + float2(0.0,   y)).rgb;
    const float3 c = in_color.Sample(in_uv + float2(  x,   y)).rgb;
    const float3 d = in_color.Sample(in_uv + float2( -x, 0.0)).rgb;
    const float3 e = in_color.Sample(in_uv).rgb;
    const float3 f = in_color.Sample(in_uv + float2(  x, 0.0)).rgb;
    const float3 g = in_color.Sample(in_uv + float2( -x,  -y)).rgb;
    const float3 h = in_color.Sample(in_uv + float2(0.0,  -y)).rgb;
    const float3 i = in_color.Sample(in_uv + float2(  x,  -y)).rgb;

    return float4((
        e * 4.0 +
        (b + d + f + h) * 2.0 +
        (a + c + g + i)
    ) / 16.0, alpha);
}

