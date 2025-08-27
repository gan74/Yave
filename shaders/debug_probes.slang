#include "lib/utils.slang"
#include "lib/sh.slang"

[[vk::binding(0)]]
Sampler2D probes;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;


struct PixelStageIn {
    float3 normal;
    uint instance_index;
};

struct VertexStageOut {
    float4 sv_position  : SV_Position;
    PixelStageIn vert;
};




int3 compute_coord(uint instance_index) {
    const int3 coord = uint3(
        (instance_index / (32 * 32)) % 32,
        (instance_index / 32) % 32,
        instance_index % 32
    );
    return coord;
}


[shader("vertex")]
VertexStageOut vert_main(StdVertexStageIn in) {
    const float3 position = int3(camera.position) + compute_coord(semantics.instance_index) - int3(15);

    VertexStageOut out;
    {
        out.sv_position = mul(camera.curr.view_proj, float4(position + in.position * 0.2, 1.0));
        out.vert.normal = unpack_2_10_10_10(in.packed_normal_tangent_sign.x).xyz;
        out.vert.instance_index = semantics.instance_index;
    }

    return out;
}


[shader("fragment")]
float4 frag_main(PixelStageIn in) {
    const uint2 atlas_coord = uint2(in.instance_index / 256, in.instance_index % 256);

    const float2 uv = saturate(octahedron_encode(in.normal));
    const float4 irr = probes[clamp(uv * 32, 0, 31) + atlas_coord * 32];

    return float4(reinhard(irr.rgb), 1.0);

}
