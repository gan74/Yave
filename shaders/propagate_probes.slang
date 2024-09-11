#include "lib/utils.slang"
#include "lib/semantics.slang"
#include "lib/sh.slang"

// -------------------------------- I/O --------------------------------

[[vk::binding(0)]]
StructuredBuffer<GIProbe> in_probes;

[[vk::binding(1)]]
RWStructuredBuffer<GIProbe> out_probes;

// -------------------------------- MAIN --------------------------------

SHRgb read_probe_sh(GIProbe probe) {
    return SHRgb(SH(probe.sh_r), SH(probe.sh_g), SH(probe.sh_b));
}

[shader("compute")]
[numthreads(1, 1, 1)]
void comp_main() {
    const int row_size = int(semantics.work_group_count.x);
    const int probe_index = int(semantics.work_group_index);

    const GIProbe probe = in_probes[probe_index];

    float weight = 1.0;
    SHRgb sh = read_probe_sh(probe);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            const int index = probe_index + y * row_size + x;
            if(index >= 0 && index < semantics.work_group_count.x * semantics.work_group_count.y) {
                const GIProbe neighbor_probe = in_probes[index];
                if(distance(neighbor_probe.position, probe.position) < 10.0) {
                    sh = sh + read_probe_sh(neighbor_probe);
                    weight += 1.0;
                }
            }
        }
    }

    sh = sh * (1.0 / weight);

    out_probes[probe_index] = probe;
    out_probes[probe_index].sh_r = sh.r;
    out_probes[probe_index].sh_g = sh.g;
    out_probes[probe_index].sh_b = sh.b;
}

