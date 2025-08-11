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
RWStructuredBuffer<uint> out_count;

[[vk::binding(5)]]
cbuffer Params {
    uint frame_id;
};



[[vk::binding(0, 1)]]
Sampler2D all_textures[];



static const uint trace_count = 16;


[shader("compute")]
[numthreads(4, 4, 4)]
void comp() {
    const uint3 size = image_size(in_grid);
    const uint3 coord = semantics.global_id;

    if(any(coord >= size)) {
        return;
    }

    if(in_grid[coord] != frame_id) {
        return;
    }

    uint index = 0;
    InterlockedAdd(out_count[0], 1, index);



    const float3 position = int3(camera.position) + coord - int3(32);


    SHRgb sh;
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

        if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
            const uint material_id = query.CommittedInstanceID();
            const MaterialData material = materials[material_id];
            const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
            const float4 albedo = all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);

            sh = sh + (sh_base * albedo.rgb);
        }
    }
}


