#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/sh.slang"
#include "lib/gi.slang"

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
RWStructuredBuffer<GIProbe> probes;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D in_rt0;

[[vk::binding(4)]]
Sampler2D in_rt1;

[[vk::binding(5)]]
Sampler2D in_lit;

[[vk::binding(6)]]
SamplerCube in_envmap;

[[vk::binding(7)]]
RaytracingAccelerationStructure tlas;



static const float min_dist = 0.001;
static const float max_dist = 100.0;


groupshared SHRgb all_samples[PROBE_SAMPLE_COUNT];
groupshared uint total_hits;

[shader("compute")]
[numthreads(PROBE_SAMPLE_COUNT, 1, 1)]
void main() {
    if(semantics.local_index == 0) {
        total_hits = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    const uint probe_index = semantics.work_group_id.y;
    const float3 probe_position = probes[probe_index].position;

    const float3 sample_dir = fibonacci_sphere(semantics.local_index, PROBE_SAMPLE_COUNT);
    const float dist = trace_first_hit(tlas, probe_position, sample_dir, min_dist, max_dist);

    const SH sh_base = SH.around(sample_dir);
    SHRgb sh;
    if(dist < max_dist) {
        const float3 hit_world_pos = probe_position + sample_dir * dist;
        const float3 hit_proj_pos = project(hit_world_pos, camera.view_proj);

        if(all(saturate(hit_proj_pos) == hit_proj_pos)) {
            const float3 gbuffer_pos = unproject(hit_proj_pos.xy, in_depth.SampleLevel(hit_proj_pos.xy, 0), camera.inv_view_proj);
            if(distance(gbuffer_pos, hit_world_pos) < 0.01) {
                const SurfaceInfo surface = decode_gbuffer({
                    float4(0.0),
                    in_rt1.SampleLevel(hit_proj_pos.xy, 0)
                });

                const float NoL = saturate(dot(surface.normal, sample_dir));
                const float3 lit_color = in_lit.SampleLevel(hit_proj_pos.xy, 0).rgb * (1.0 / PROBE_SAMPLE_COUNT);


                sh = sh_base * lit_color;
                InterlockedAdd(total_hits, 1);
            }
        }
    } else {
        sh = sh_base * in_envmap.SampleLevel(sample_dir, 0).rgb;
    }

    all_samples[semantics.local_index] = sh;

    GroupMemoryBarrierWithGroupSync();

    for(uint k = 2; k <= PROBE_SAMPLE_COUNT; k *= 2) {
        if(semantics.local_index % k == 0) {
            const uint i = semantics.local_index + k / 2;
            if(i < PROBE_SAMPLE_COUNT) {
                all_samples[semantics.local_index] = all_samples[semantics.local_index] + all_samples[i];
            }
        }

        GroupMemoryBarrierWithGroupSync();
    }


    if(semantics.local_index == 0) {
        probes[probe_index].sh_r = all_samples[0].r;
        probes[probe_index].sh_g = all_samples[0].g;
        probes[probe_index].sh_b = all_samples[0].b;
        probes[probe_index].hits = float(total_hits);
    }
}
