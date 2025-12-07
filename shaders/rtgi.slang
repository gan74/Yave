#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/hash.slang"

[[vk::binding(0)]]
RWTexture2D out_image;

[[vk::binding(1)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(2)]]
ConstantBuffer<Camera> camera;

[[vk::binding(3)]]
Sampler2D<float> in_depth;

[[vk::binding(4)]]
Sampler2D<uint> in_rt1;

[[vk::binding(5)]]
Sampler2D in_lit;

[[vk::binding(6)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(7)]]
SamplerCube in_envmap;

[[vk::binding(8)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(9)]]
cbuffer Params {
    uint sample_count;
    uint resolution_scale;
    uint frame_id;
    uint padding;
};

[[vk::binding(0, 1)]]
Sampler2D all_textures[];



bool is_on_screen(float3 world_pos, out float2 screen_uv) {
    const float3 proj = project(world_pos, camera.curr.view_proj);
    screen_uv = proj.xy;

    if(any(saturate(proj.xy) != proj.xy)) {
        return false;
    }

    return in_depth.SampleLevel(proj.xy, 0) <= proj.z + constants.epsilon;
}

float4 eval_material_color(uint instance_id) {
    const MaterialData material = materials[instance_id];
    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
    return all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);
}

float4 eval_hit_color(HitInfo hit) {
    float2 hit_uv;
    if(is_on_screen(hit.position, hit_uv)) {
        return float4(in_lit.SampleLevel(hit_uv, 0.0).rgb, 1.0);
    }

    return eval_material_color(hit.instance_id);
}


[shader("compute")]
[numthreads(8, 8)]
void comp() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 scaled_coord = coord << resolution_scale;
    const uint2 size = image_size(in_depth);

    if(any(scaled_coord >= size)) {
        return;
    }

    const float2 px_center = float2(scaled_coord) + float2(0.5);
    const float2 in_uv = px_center / float2(size);

    const float depth = in_depth[scaled_coord];
    const SurfaceInfo surface = decode_gbuffer(RawGBuffer(
        float4(0.0),
        in_rt1[scaled_coord],
    ));

    const float3 world_pos = unproject(in_uv, depth, camera.curr.inv_view_proj);

#if 0
    const float3 view_dir = normalize(world_pos - camera.position);
    const HitInfo hit = trace_inline(tlas, world_pos, reflect(view_dir, surface.normal), 0.001, 10000.0);
    out_image[coord] = eval_hit_color(hit);
#else
    float3 acc = float3(0.0);

    const uint sample_index_offset = hash.xorshift(semantics.global_index + hash.xorshift(frame_id));
    for(uint k = 0; k != sample_count; ++k) {
        const float3 trace_dir = sample_hemisphere(surface.normal, sample_index_offset * sample_count + k);

        const float tmin = 0.001;
        const float tmax = 10000.0;

        const HitInfo hit = trace_inline(tlas, world_pos, trace_dir, tmin, tmax);

        if(hit.hit) {
            const DirectionalLight light = lights[0];
            if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
                const float3 normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
                const float NoL = dot(light.direction, normal);
                if(NoL > 0.0) {
                    const float3 view_dir = normalize(world_pos - camera.position);
                    const float4 albedo = eval_material_color(hit.instance_id);
                    acc += albedo.rgb * light.color * NoL * dot(surface.normal, trace_dir)/* * dot(surface.normal, view_dir)*/;
                }
            }
        } else {
            acc += in_envmap.SampleLevel(trace_dir, 0.0).rgb;
        }
    }

    out_image[coord] = float4(acc / sample_count, 1.0);
#endif
}


