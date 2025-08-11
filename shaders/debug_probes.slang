#include "lib/utils.slang"
#include "lib/sh.slang"

[[vk::binding(0)]]
Sampler3D<uint> in_grid;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(3)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(4)]]
cbuffer Params {
    uint frame_id;
};



[[vk::binding(0, 1)]]
Sampler2D all_textures[];





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
        (instance_index / 4096) % 64,
        (instance_index / 64) % 64,
        instance_index % 64
    );
    return coord;
}


[shader("vertex")]
VertexStageOut vert_main(StdVertexStageIn in) {
    const int3 coord = compute_coord(semantics.instance_index);
    const float3 position = int3(camera.position) + coord - int3(32);

    VertexStageOut out;
    {
        out.sv_position = mul(camera.curr.view_proj, float4(position + in.position * 0.2, 1.0));
        out.vert.normal = unpack_2_10_10_10(in.packed_normal_tangent_sign.x).xyz;
        out.vert.instance_index = semantics.instance_index;
    }

    if(in_grid[coord] != frame_id) {
        out.sv_position.x = asfloat(0xFFFFFFFF);
    }

    return out;
}


[shader("fragment")]
float4 frag_main(PixelStageIn in) {
    // return float4(heat_spectrum(float(in.probe_index % 1024) / 1024.0), 1.0);

    const int3 coord = compute_coord(in.instance_index);
    const float3 position = int3(camera.position) + coord - int3(32);

    /*SHRgb sh;
    for(uint i = 0; i != trace_count; ++i) {
        const float3 sample_dir = fibonacci_sphere(i, trace_count);
        const SH sh_base = SH.around(sample_dir);

        RayDesc ray;
        {
            ray.Origin = position;
            ray.Direction = sample_dir;
            ray.TMin = 0.001;
            ray.TMax = 10000.0;
        }

        RayQuery<RAY_FLAG_FORCE_OPAQUE> query;
        query.TraceRayInline(tlas, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

        while(query.Proceed());

        float3 hit_color = float3(0.0);
        if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
            const uint material_id = query.CommittedInstanceID();
            const MaterialData material = materials[material_id];
            const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
            const float4 albedo = all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);
            hit_color = albedo.rgb / trace_count;
        }

        sh = sh + (sh_base * hit_color);
    }

    return float4(sh.unproject(in.normal), 1.0);*/

    RayDesc ray;
    {
        ray.Origin = position;
        ray.Direction = in.normal;
        ray.TMin = 0.001;
        ray.TMax = 10000.0;
    }

    RayQuery<RAY_FLAG_FORCE_OPAQUE> query;
    query.TraceRayInline(tlas, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

    while(query.Proceed());

    if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        const uint material_id = query.CommittedInstanceID();
        const MaterialData material = materials[material_id];
        const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
        const float4 albedo = all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);
        return albedo;
    }

    return float4(0.0);

}
