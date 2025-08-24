#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/reservoir.slang"

[[vk::binding(0)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D<uint> in_rt1;

[[vk::binding(4)]]
Sampler2D in_lit;

[[vk::binding(5)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(6)]]
SamplerCube in_envmap;

[[vk::binding(7)]]
cbuffer SampleDirData { float4 sample_dirs[256]; };

[[vk::binding(8)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(9)]]
RWTexture2D out_image;

[[vk::binding(10)]]
cbuffer Params {
    uint sample_count;
    uint resolution_scale;
    uint restir;
};

[[vk::binding(0, 1)]]
Sampler2D all_textures[];



bool is_on_screen(float3 world_pos, out float2 screen_uv) {
    const float3 proj = project(world_pos, camera.curr.view_proj);
    screen_uv = proj.xy;

    if(any(saturate(proj.xy) != proj.xy)) {
        return false;
    }

    return in_depth.SampleLevel(proj.xy, 0) <= proj.z + epsilon;
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


void init_restir<T: Rng>(inout Reservoir res, float3 normal, inout T rng, uint M = 32) {
    float pdf = 1.0 / 256.0;

    //initial selection of 1 light of M
    for(uint i = 0; i != M; ++i) {
        const uint sample_index = uint(rng.next_float() * 255);
        const float p_hat = dot(normal, sample_dirs[sample_index].xyz) + 1.0; // TEMP
        const float w = p_hat / pdf;
        res.update(sample_index, w, 1, rng);
    }

    /*const uint sample_index = res.Y;
    if(sample_index < 256) {
        const float3 sample = sample_dirs[sample_index].xyz;

        return sample;

        //reservoir.W_y = p_hat > 0.0 ? rcp(p_hat) * reservoir.W_sum / reservoir.M : 0.0;
        //radiance *=  reservoir.W_y;
    }
    return sample_dirs[0].xyz;*/
}


[shader("compute")]
[numthreads(8, 8)]
void comp() {
    const uint2 size = image_size(in_depth);
    const uint2 coord = semantics.global_id.xy << resolution_scale;

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
    const float3 view_dir = normalize(world_pos - camera.position);
    const HitInfo hit = trace_inline(tlas, world_pos, reflect(view_dir, surface.normal), 0.001, 10000.0);
    out_image[semantics.global_id.xy] = eval_hit_color(hit);
#else
    float3 acc = float3(0.0);

    HashRng rng = HashRng(hash(coord.x << 16 | coord.y));
    Reservoir reservoir;

    for(uint k = 0; k != sample_count; ++k) {
        float3 sample_dir = sample_dirs[hash(uint2(semantics.work_group_index, semantics.local_index * sample_count + k)) & 0xFF].xyz;
        if(restir != 0) {
            init_restir(reservoir, surface.normal, rng);
            const uint sample_index = reservoir.Y;
            if(sample_index < 256) {
                sample_dir = sample_dirs[sample_index].xyz;
            } else {
                break;
            }
        }

        const float3 trace_dir = dot(surface.normal, sample_dir) < 0.0 ? -sample_dir : sample_dir;

        const float tmin = 0.001;
        const float tmax = 10000.0;

        const HitInfo hit = trace_inline(tlas, world_pos, trace_dir, tmin, tmax);

        float3 contrib = float3(0.0);
        if(hit.hit) {
            const DirectionalLight light = lights[0];
            if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
                const float3 normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
                const float NoL = dot(light.direction, normal);
                if(NoL > 0.0) {
                    const float4 albedo = eval_material_color(hit.instance_id);
                    contrib = albedo.rgb * light.color * NoL * dot(surface.normal, trace_dir);
                }
            }
        } else {
            contrib = in_envmap.SampleLevel(trace_dir, 0.0).rgb;
        }

        if(restir != 0) {
            contrib *= min(10.0, reservoir.reintegrate_sample(length(contrib)));
        }

        acc += contrib;
    }

    out_image[semantics.global_id.xy] = float4(reinhard(acc / sample_count), 1.0);
#endif
}


