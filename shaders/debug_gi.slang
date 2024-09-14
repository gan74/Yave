#include "lib/utils.slang"
#include "lib/sh.slang"
#include "lib/gi.slang"


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
StructuredBuffer<GIProbe> in_probes;

[[vk::binding(5)]]
RWTexture2D out_color;

[[vk::binding(6)]]
cbuffer Params_Inline {
    uint2 grid_size;
}

// -------------------------------- MAIN --------------------------------

[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const int2 coord = int2(semantics.global_id.xy);
    const float2 uv = (semantics.global_id.xy + 0.5) / float2(image_size(in_depth).xy);
    const float3 world_pos = unproject(uv, in_depth[coord], camera.inv_view_proj);

    const int2 probe_coord = coord / PROBE_PIXEL_SIZE;
    const uint probe_index = probe_coord.x + grid_size.x * probe_coord.y;

    const SurfaceInfo surface = decode_gbuffer({
        in_rt0[coord],
        in_rt1[coord],
    });

#if 1
    static const int kernel_size = 1;
    static const float max_dist = 2.0;

    float3 acc = float3(0.0);
    float weight = 0.0;
    for(int x = -kernel_size; x <= kernel_size; ++x) {
        for(int y = -kernel_size; y <= kernel_size; ++y) {
            const int index = probe_index + y * grid_size.x + x;
            if(index >= 0 && index < grid_size.x * grid_size.y) {
                const GIProbe probe = in_probes[index];
                const float dist = distance(probe.position, world_pos);
                if(dist < max_dist) {
                    const float w = 1.0 / (dist + 1.0);
                    const SHRgb sh = SHRgb(SH(probe.sh_r), SH(probe.sh_g), SH(probe.sh_b));
                    acc += sh.unproject(surface.normal) * w;
                    weight += w;
                }
            }
        }
    }
    out_color[coord] = float4(weight <= 0.0 ? float3(0.0) : acc / weight, 1.0);
#else
    const GIProbe probe = in_probes[probe_index];
    const SHRgb sh = SHRgb(SH(probe.sh_r), SH(probe.sh_g), SH(probe.sh_b));
    out_color[coord] = float4(sh.unproject(surface.normal), 1.0);
#endif
}

