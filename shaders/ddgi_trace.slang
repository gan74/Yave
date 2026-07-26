#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/rng.slang"
#include "lib/ddgi.slang"


[[vk::binding(0)]]
RWTexture2D out_radiance;

[[vk::binding(1)]]
RWTexture2D<float2> out_distance;

[[vk::binding(2)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(3)]]
SamplerCube in_envmap;

[[vk::binding(4)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(5)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(6)]]
cbuffer Params {
    float probe_spacing;
    uint light_count;
    uint frame_id;
    uint reset;
    uint probe_update_stride;
    float blend_alpha;
    float padding_0;
};


[[vk::binding(0, 1)]]
Sampler2D all_textures[];



struct DDGIMaterial {
    float3 albedo;
    float3 emissive;
};

DDGIMaterial eval_material(uint instance_id) {
    const MaterialData material = materials[instance_id];
    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
    const uint emissive_id = material.texture_indices[uint(TextureSlots::Emissive)];
    return DDGIMaterial(
        all_textures[NonUniformResourceIndex(albedo_id)].SampleLevel(float2(0.5), 100.0).rgb * material.base_color_factor,
        all_textures[NonUniformResourceIndex(emissive_id)].SampleLevel(float2(0.5), 100.0).rgb * material.emissive_factor,
    );
}

float3 shade_hit(HitInfo hit, float3 trace_dir) {
    if(!hit.hit) {
        return in_envmap.SampleLevel(trace_dir, 0.0).rgb;
    }

    const float3 hit_normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
    const DDGIMaterial material = eval_material(hit.instance_id);
    const float n_t = constants.inv_pi;

    float3 radiance = material.albedo * in_envmap.SampleLevel(hit_normal, 999.0).rgb * n_t;

    const float tmin = 0.001;
    const float tmax = 10000.0;
    for(uint k = 0; k != light_count; ++k) {
        const DirectionalLight light = lights[k];
        const float n_l = dot(light.direction, hit_normal);
        if(n_l > 0.0) {
            if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
                radiance += material.albedo * light.color * n_l * n_t;
            }
        }
    }

    return radiance;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main() {
    const uint probe_index = semantics.global_id.z;
    if(probe_index >= ddgi.probe_count) {
        return;
    }

    if(reset == 0 && !ddgi.should_update_probe(probe_index, frame_id, probe_update_stride)) {
        return;
    }

    const uint2 probe_local = semantics.global_id.xy;
    const uint2 coord = ddgi.radiance_atlas_coord(probe_index) + probe_local;

    const int3 grid_coord = ddgi.compute_grid_coord(probe_index);
    const float3 origin = ddgi.probe_world_position(grid_coord, probe_spacing);

    const uint seed = hash.xorshift(semantics.global_index + hash.xorshift(frame_id));
    HashRng rng = HashRng(seed);

    const float2 octa_uv = (float2(probe_local) + float2(rng.next_unorm(), rng.next_unorm())) / float(ddgi.radiance_probe_size);
    const float3 trace_dir = octahedron_decode(octa_uv);

    const float tmin = 0.01;
    const float tmax = 10000.0;
    const HitInfo hit = trace_inline(tlas, origin, trace_dir, tmin, tmax);

    const float3 new_radiance = shade_hit(hit, trace_dir);
    const float2 new_distance = float2(hit.ray_t, sqr(hit.ray_t));

    if(reset != 0) {
        out_radiance[coord] = float4(new_radiance, 1.0);
        out_distance[coord] = new_distance;
    } else {
        out_radiance[coord] = float4(lerp(out_radiance[coord].rgb, new_radiance, blend_alpha), 1.0);
        out_distance[coord] = lerp(out_distance[coord], new_distance, blend_alpha);
    }
}
