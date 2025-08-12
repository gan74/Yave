import "glsl";

#include "lib/utils.slang"

[[vk::binding(0)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D<uint> in_rt1;

[[vk::binding(4)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(5)]]
SamplerCube in_envmap;

[[vk::binding(6)]]
cbuffer SampleDirData { float4 sample_dirs[256]; };

[[vk::binding(7)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(8)]]
RWTexture2D out_image;

[[vk::binding(9)]]
cbuffer Params {
    uint directional_lights;
};

[[vk::binding(0, 1)]]
Sampler2D all_textures[];


bool hit(RaytracingAccelerationStructure accel, float3 origin, float3 dir, float min_dist, float max_dist) {
    rayQueryEXT query;
    rayQueryInitializeEXT(
        query,
        accel,
        gl_RayFlagsOpaqueEXT,
        0xFF,
        origin,
        min_dist,
        dir,
        max_dist
     );

    rayQueryProceedEXT(query);

    return rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}



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
        float4(0.0),
        in_rt1[coord],
    });

    const float3 world_pos = unproject(in_uv, depth, camera.curr.inv_view_proj);

    float3 acc = float3(0.0);

    const uint sample_count = 4;
    for(uint k = 0; k != sample_count; ++k) {
        const float3 sample_dir = sample_dirs[(hash(semantics.work_group_index) ^ hash(semantics.local_index) ^ hash(k)) & 0xFF].xyz;
        const float3 trace_dir = dot(surface.normal, sample_dir) < 0.0 ? -sample_dir : sample_dir;

        const float tmin = 0.001;
        const float tmax = 10000.0;

        rayQueryEXT query;
        rayQueryInitializeEXT(
            query,
            tlas,
            gl_RayFlagsOpaqueEXT,
            0xFF,
            world_pos,
            tmin,
            trace_dir,
            tmax
        );

        rayQueryProceedEXT(query);

        if(rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionTriangleEXT) {
            const uint material_id = rayQueryGetIntersectionInstanceCustomIndexEXT(query, true);
            const float3 hit_pos = world_pos + trace_dir * rayQueryGetIntersectionTEXT(query, true);

            const DirectionalLight light = lights[0];
            if(!hit(tlas, hit_pos, light.direction, tmin, tmax)) {
                const MaterialData material = materials[material_id];
                const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
                const float4 albedo = all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);

                acc += albedo.rgb * light.color;
            }

        } else {
            acc += in_envmap.SampleLevel(trace_dir, 0.0).rgb;
        }
    }

    out_image[coord] = float4(acc / sample_count, 1.0);

}


