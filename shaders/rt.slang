import "glsl";

#include "lib/utils.slang"

[[vk::binding(0)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
RWTexture2D out_image;



struct RayPayload {
    float3 hit_value;
};




[shader("compute")]
void comp() {
    const uint2 size = image_size(out_image);
    const uint2 coord = uint2(semantics.global_id.xy);
    const float2 px_center = float2(coord) + float2(0.5);

    if(any(coord >= size)) {
        return;
    }

    const float2 uv = px_center / float2(size);

    const float3 target = unproject(uv, 1.0, camera.cur.inv_view_proj);
    const float3 dir = normalize(target - camera.position);

    const float tmin = 0.001;
    const float tmax = 10000.0;

    rayQueryEXT ray_query;
    rayQueryInitializeEXT(
        ray_query,              // Ray query
        tlas,                   // Top-level acceleration structure
        gl_RayFlagsOpaqueEXT,   // Ray flags, here saying "treat all geometry as opaque"
        0xFF,                   // 8-bit instance mask, here saying "trace against all instances"
        camera.position,        // Ray origin
        tmin,                   // Minimum t-value
        dir,                    // Ray direction
        tmax                    // Maximum t-value
    );

    while(rayQueryProceedEXT(ray_query)) {
    }

    if(rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT) {
        const uint instance_id = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, true);
        out_image[coord] = float4(heat_spectrum(float(hash(instance_id) % 4096) / 4096), 0.0);
    } else {
        out_image[coord] = 0.0;
    }

}


