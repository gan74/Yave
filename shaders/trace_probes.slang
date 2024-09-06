#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/sh.slang"

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
RWStructuredBuffer<GIProbe> probes;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D in_lit;

[[vk::binding(4)]]
RaytracingAccelerationStructure tlas;



static const float min_dist = 0.001;
static const float max_dist = 10.0;


#define SAMPLE_COUNT 256


groupshared SHRgb all_samples[SAMPLE_COUNT];

[shader("compute")]
[numthreads(SAMPLE_COUNT, 1, 1)]
void main() {
    const uint probe_index = semantics.work_group_id.y;
    const float3 probe_position = probes[probe_index].position;

    const float3 sample_dir = fibonacci_sphere(semantics.local_index, SAMPLE_COUNT);
    const float dist = trace_first_hit(tlas, probe_position, sample_dir, min_dist, max_dist);

    SHRgb sh;
    float weight = 0.0;
    if(dist < max_dist) {
        const float3 hit_world_pos = probe_position + sample_dir * dist;
        const float3 hit_proj_pos = project(hit_world_pos, camera.view_proj);

        if(all(saturate(hit_proj_pos) == hit_proj_pos)) {
            const float3 lit_color = in_lit.SampleLevel(hit_proj_pos.xy, 0).rgb / (sqr(dist) * SAMPLE_COUNT);
            sh = SH.around(sample_dir) * lit_color;
        }
    }

    all_samples[semantics.local_index] = sh;

    GroupMemoryBarrierWithGroupSync();

    for(uint k = 2; k <= SAMPLE_COUNT; k *= 2) {
        if(semantics.local_index % k == 0) {
            const uint i = semantics.local_index + k / 2;
            if(i < SAMPLE_COUNT) {
                all_samples[semantics.local_index] = all_samples[semantics.local_index] + all_samples[i];
            }
        }

        GroupMemoryBarrierWithGroupSync();
    }


    if(semantics.local_index == 0) {
        probes[probe_index].sh_r = all_samples[0].r;
        probes[probe_index].sh_g = all_samples[0].g;
        probes[probe_index].sh_b = all_samples[0].b;
    }
}
