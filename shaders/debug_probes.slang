#include "lib/utils.slang"
#include "lib/sh.slang"

[[vk::binding(0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(1)]]
StructuredBuffer<GIProbe> probes;

struct PixelStageIn {
    float3 normal;
    uint probe_index;
};

struct VertexStageOut {
    float4 sv_position  : SV_Position;
    PixelStageIn vert;
};



[shader("vertex")]
VertexStageOut vert_main(StdVertexStageIn in) {
    const GIProbe probe = probes[semantics.instance_index];
    const float dist = length(probe.position - camera.position);

    VertexStageOut out;
    {
        out.sv_position = mul(camera.view_proj, float4(probe.position + in.position * dist / 60.0, 1.0));
        out.vert.normal = unpack_2_10_10_10(in.packed_normal_tangent_sign.x).xyz;
        out.vert.probe_index = semantics.instance_index;
    }

    return out;
}



[shader("fragment")]
float4 frag_main(PixelStageIn in) {
    const GIProbe probe = probes[in.probe_index];
    const SHRgb sh = SHRgb(SH(probe.sh_r), SH(probe.sh_g), SH(probe.sh_b));
    return float4(sh.unproject(normalize(in.normal)), 1.0);
}
