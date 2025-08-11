import "glsl";

#include "lib/utils.slang"

[[vk::binding(0)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D in_rt0;

[[vk::binding(4)]]
Sampler2D<uint> in_rt1;

[[vk::binding(5)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(6)]]
SamplerCube in_envmap;

[[vk::binding(7)]]
RWTexture2D out_image;

[[vk::binding(0, 1)]]
Sampler2D all_textures[];




struct RayPayload {
    float3 hit_value;
};




[shader("compute")]
void comp() {
    const uint2 size = image_size(out_image);
    const uint2 coord = uint2(semantics.global_id.xy);

    if(any(coord >= size)) {
        return;
    }


    const float2 px_center = float2(coord) + float2(0.5);
    const float2 in_uv = px_center / float2(size);

    const float depth = in_depth[coord];
    const SurfaceInfo surface = decode_gbuffer({
        in_rt0[coord],
        in_rt1[coord],
    });

    const float3 world_pos = unproject(in_uv, depth, camera.cur.inv_view_proj);



    const float3 target = unproject(in_uv - camera.jitter * 0.5, 1.0, camera.cur.inv_view_proj);
    const float3 dir = normalize(target - camera.position);
    const float3 reflected_dir = reflect(dir, surface.normal);

    const float tmin = 0.001;
    const float tmax = 10000.0;

    rayQueryEXT ray_query;
    rayQueryInitializeEXT(
        ray_query,                              // Ray query
        tlas,                                   // Top-level acceleration structure
        gl_RayFlagsOpaqueEXT,                   // Ray flags, here saying "treat all geometry as opaque"
        0xFF,                                   // 8-bit instance mask, here saying "trace against all instances"
        world_pos + surface.normal * tmin,      // Ray origin
        tmin,                                   // Minimum t-value
        reflected_dir,                          // Ray direction
        tmax                                    // Maximum t-value
    );

    while(rayQueryProceedEXT(ray_query)) {
    }

    float3 hit_color;
    if(rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT) {
        const uint material_id = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, true);
        const MaterialData material = materials[material_id];
        const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
        const float4 albedo = all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);
        hit_color = albedo.rgb * surface.albedo;
        
    } else {
        hit_color = in_envmap.SampleLevel(reflected_dir, 0.0).rgb;
    }

    out_image[coord] = float4(hit_color, 1.0);

}


