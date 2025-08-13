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


struct HitInfo {
    bool hit;
    float ray_t;
    float3 position;
    uint instance_id;
    uint primitive_id;
    float2 barycentrics;
    float3 tri_vertices[3];

    __init() {
        hit = false;
        ray_t = 0.0;
        position = float3(0.0);
        instance_id = primitive_id = 0;
        barycentrics = float2(0.0);
        tri_vertices = {float3(0.0), float3(0.0), float3(0.0)};
    }

    float3 normal() {
        return normalize(cross((tri_vertices[0] - tri_vertices[1]), (tri_vertices[0] - tri_vertices[2])));
    }
};

HitInfo trace_inline(RaytracingAccelerationStructure accel, float3 origin, float3 dir, float min_dist, float max_dist) {
    RayDesc ray;
    {
        ray.Origin = origin;
        ray.Direction = dir;
        ray.TMin = min_dist;
        ray.TMax = max_dist;
    }

    RayQuery<RAY_FLAG_FORCE_OPAQUE> query;
    query.TraceRayInline(accel, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

    query.Proceed();

    HitInfo info;
    if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        info.hit = true;
        info.ray_t = query.CommittedRayT();
        info.position = origin + dir * info.ray_t;
        info.instance_id = query.CommittedInstanceID();
        info.primitive_id = query.CommittedPrimitiveIndex();
        info.barycentrics = query.CommittedTriangleBarycentrics();
        info.tri_vertices = query.CandidateGetIntersectionTriangleVertexPositions();
    }

    return info;
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

#if 0
    const HitInfo hit = trace_inline(tlas, camera.position, normalize(world_pos - camera.position), 0.001, 10000.0);
    out_image[coord] = float4(max(0.0, dot(hit.normal(), lights[0].direction)).xxx, 0.0);
#endif

    float3 acc = float3(0.0);
    const uint sample_count = 16;
    for(uint k = 0; k != sample_count; ++k) {
        const float3 sample_dir = sample_dirs[hash(uint2(semantics.work_group_index, semantics.local_index * sample_count + k)) & 0xFF].xyz;
        const float3 trace_dir = dot(surface.normal, sample_dir) < 0.0 ? -sample_dir : sample_dir;

        const float tmin = 0.001;
        const float tmax = 10000.0;
        
        const HitInfo hit = trace_inline(tlas, world_pos, trace_dir, tmin, tmax);

        if(hit.hit) {
            const DirectionalLight light = lights[0];
            if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
                const float NoL = dot(light.direction, hit.normal());
                if(NoL > 0.0) {
                    const MaterialData material = materials[hit.instance_id];
                    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
                    const float4 albedo = all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);

                    acc += albedo.rgb * light.color * NoL;
                }
            }
        } else {
            acc += in_envmap.SampleLevel(trace_dir, 0.0).rgb;
        }
    }

    out_image[coord] = float4(acc / sample_count, 1.0);

}


