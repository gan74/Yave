[[vk::binding(0)]]
Sampler2D in_color;

[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    return in_color.Sample(in_uv);
}
