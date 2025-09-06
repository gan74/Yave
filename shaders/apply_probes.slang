// #define DDGI_TRACE_PROBES

#include "lib/ddgi.slang"

[[vk::binding(0)]]
RWTexture2D<float3> out_image;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
Sampler2D<float3> in_irradiance;

[[vk::binding(4)]]
Sampler2D<float2> in_distance;

[[vk::binding(5)]]
ConstantBuffer<Camera> camera;

[[vk::binding(6)]]
RaytracingAccelerationStructure tlas;



[shader("compute")]
[numthreads(8, 8)]
void comp() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(out_image);

    if(any(coord > size)) {
        return;
    }

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        out_image[coord] = float3(0.0);
        return;
    }

    const float3 world_pos = unproject(((semantics.global_id.xy + 0.5) / size), depth, camera.curr.inv_view_proj);
    if(!ddgi.in_range(world_pos, camera.position)) {
        out_image[coord] = float3(0.0);
        return;
    }


    const SurfaceInfo surface = decode_gbuffer({float4(0.0), in_rt1[coord]});

#ifdef DDGI_TRACE_PROBES
    const float3 irr = sample_irradiance(world_pos, surface.normal, camera.position, in_irradiance, tlas);
#else
    const float3 irr = sample_irradiance(world_pos, surface.normal, camera.position, in_irradiance, in_distance);
#endif

    out_image[coord] = irr;
}
