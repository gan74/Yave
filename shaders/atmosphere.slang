#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2D in_color;

[[vk::binding(2)]]
ConstantBuffer<Camera> camera;

[[vk::binding(3)]]
cbuffer Atmosphere_Inline {
    float3 sun_color;
    float density;
};


[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const int2 coord = int2(semantics.frag_coord.xy);
    const float depth = in_depth[coord];
    const float3 color = in_color[coord].rgb;

    if(is_depth_OOB(depth)) {
        return float4(color, 1.0);
    }

    const float3 view_vec = unproject(in_uv, max(epsilon, depth), camera.inv_view_proj) - camera.position;
    const float view_dist = length(view_vec);

    const float d = saturate(exp(-view_dist * density));
    return float4(lerp(sun_color, color, d), 1.0);
}

