#include "lib/utils.slang"
#include "lib/sh.slang"
#include "lib/gi.slang"

#define DEBUG
#include "lib/debug.slang"

// -------------------------------- I/O --------------------------------

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2D in_rt0;

[[vk::binding(2)]]
Sampler2D in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
StructuredBuffer<GIProbe> probes;

[[vk::binding(5)]]
RWTexture2D out_color;


// -------------------------------- MAIN --------------------------------

[shader("compute")]
[numthreads(PROBE_PIXEL_SIZE, PROBE_PIXEL_SIZE)]
void comp_main() {
    const int2 coord = int2(semantics.global_id.xy);
    const SurfaceInfo surface = decode_gbuffer({
        in_rt0[coord],
        in_rt1[coord],
    });


    const GIProbe probe = probes[semantics.work_group_index];
    const SHRgb sh = SHRgb(SH(probe.sh_r), SH(probe.sh_g), SH(probe.sh_b));

    // float3 heat = sh.unproject(surface.normal); //heat_spectrum(probe.hits / PROBE_SAMPLE_COUNT);
    // heat = lerp(heat, float3(1.0) - heat, print_value(semantics.local_id.xy * 2.0, float2(0.0), float2(PROBE_PIXEL_SIZE / 2, PROBE_PIXEL_SIZE - 1), uint(probe.hits), 2.0, 0.0));

    out_color[coord] = float4(sh.unproject(surface.normal), 1.0);
}

