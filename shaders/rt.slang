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
[numthreads(8, 8)]
void comp() {
    const uint2 size = image_size(in_depth);
    const uint2 coord = semantics.global_id.xy;

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

#if 1
    const float3 world_pos = unproject(in_uv, depth, camera.curr.inv_view_proj);

    const float3 target = unproject(in_uv - camera.jitter * 0.5, 1.0, camera.curr.inv_view_proj);
    const float3 view_dir = normalize(target - camera.position);
    const float3 trace_dir = reflect(view_dir, surface.normal);
#else
    const float3 world_pos = camera.position;
    const float3 target = unproject(in_uv - camera.jitter * 0.5, 1.0, camera.curr.inv_view_proj);
    const float3 trace_dir = normalize(target - camera.position);
#endif

    const float tmin = 0.001;
    const float tmax = 10000.0;

    float ray_t = 0.0;
    float3 hit_color;
    rayQueryEXT ray_query;

    for(uint k = 0; k != 4; ++k) {
        rayQueryInitializeEXT(
            ray_query,
            tlas,
            gl_RayFlagsOpaqueEXT,
            0xFF,
            world_pos,
            tmin + ray_t,
            trace_dir,
            tmax
        );

        while(rayQueryProceedEXT(ray_query));

        if(rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT) {
            const uint material_id = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, true);
            const MaterialData material = materials[material_id];
            const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
            const float4 albedo = all_textures[NonUniformResourceIndex(albedo_id)].SampleLevel(float2(0.5), 100.0);
            hit_color = albedo.rgb * surface.albedo;

            if(albedo.a < 0.9) {
                const int primitive_id = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, true);
                if(hash(primitive_id) % 1024 > albedo.a * 1024.0) {
                    ray_t = rayQueryGetIntersectionTEXT(ray_query, true);
                    continue;
                }
            }
        } else {
            hit_color = in_envmap.SampleLevel(trace_dir, 0.0).rgb;
        }

        break;
    }

    out_image[coord] = float4(hit_color, 1.0);

}


