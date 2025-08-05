#include "lib/utils.slang"

struct UnalignedFloat3 {
    float x;
    float y;
    float z;

    float3 to_float3() {
        return float3(x, y, z);
    }
};


[[vk::binding(0)]]
RWTexture2D<uint> out_data;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
RWStructuredBuffer<GIMeshInfo> meshes;

[[vk::binding(3)]]
RWStructuredBuffer<UnalignedFloat3> positions;

[[vk::binding(4)]]
RWStructuredBuffer<TransformableData> transformables;

[[vk::binding(5)]]
ConstantBuffer<uint> count;


[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint index = semantics.global_id.y;
    if(index >= count) {
        return;
    }

    const GIMeshInfo mesh_info = meshes[index];
    const TransformableData transform = transformables[mesh_info.transform_index];

    const uint per_mesh_thread_count = semantics.work_group_count.x * semantics.work_group_size.x;
    const uint2 range = dispatch_range(semantics.global_id.x, mesh_info.vertex_count, per_mesh_thread_count);
    for(uint i = range.x; i != range.y; ++i) {
        const float3 model_pos = positions[mesh_info.vertex_offset + i].to_float3();
        const float3 pos = mul(transform.current, float4(model_pos, 1.0)).xyz;
        const float3 proj = project(pos, camera.cur.view_proj);

        if(proj.z < 0) {
            continue;
        }

        const uint2 coord = uint2(round(proj.xy * 128.0));

        uint prev = 0;
        InterlockedMax(out_data[coord], asuint(proj.z), prev);
    }
}
