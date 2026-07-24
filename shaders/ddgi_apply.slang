#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/ddgi.slang"


[[vk::binding(0)]]
RWTexture2D out_gi;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
Sampler2D<float3> in_ddgi_irradiance;

[[vk::binding(5)]]
Sampler2D<float2> in_ddgi_distance;

[[vk::binding(6)]]
cbuffer Params {
    float probe_spacing;
};


[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(out_gi).xy;
    if(any(coord >= size)) {
        return;
    }

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        out_gi[coord] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    const float2 uv = (float2(coord) + 0.5) / float2(size);
    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const float3 normal = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[coord])).normal;

    const float3 gi = sample_irradiance(
        world_pos,
        normal,
        camera.position,
        probe_spacing,
        in_ddgi_irradiance,
        in_ddgi_distance
    );

    out_gi[coord] = float4(gi, 1.0);
}
