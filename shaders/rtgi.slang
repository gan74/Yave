#include "lib/utils.slang"
#include "lib/rt.slang"

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
    uint sample_count;
};

[[vk::binding(0, 1)]]
Sampler2D all_textures[];


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
    const float3 normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
    out_image[coord] = float4(normal * 0.5 + 0.5, 1.0);
#endif

    float3 acc = float3(0.0);
    for(uint k = 0; k != sample_count; ++k) {
        const float3 sample_dir = sample_dirs[hash(uint2(semantics.work_group_index, semantics.local_index * sample_count + k)) & 0xFF].xyz;
        const float3 trace_dir = dot(surface.normal, sample_dir) < 0.0 ? -sample_dir : sample_dir;

        const float tmin = 0.001;
        const float tmax = 10000.0;

        const HitInfo hit = trace_inline(tlas, world_pos, trace_dir, tmin, tmax);

        if(hit.hit) {
            const DirectionalLight light = lights[0];
            if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
                const float3 normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
                const float NoL = dot(light.direction, normal);
                if(NoL > 0.0) {
                    const MaterialData material = materials[hit.instance_id];
                    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
                    const float4 albedo = all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);

                    acc += albedo.rgb * light.color * NoL * dot(surface.normal, trace_dir);
                }
            }
        } else {
            acc += in_envmap.SampleLevel(trace_dir, 0.0).rgb;
        }
    }

    out_image[coord] = float4(acc / sample_count, 1.0);

}


