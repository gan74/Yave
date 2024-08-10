#include "lib/utils.slang"
#include "lib/rt.slang"


// -------------------------------- DEFINES --------------------------------

#define SAMPLE_BUFFER_SIZE 256

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
RWTexture2D<float> out_ao;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
cbuffer SampleDirData {
    float4 sample_dirs[SAMPLE_BUFFER_SIZE];
};

[[vk::binding(5)]]
RaytracingAccelerationStructure tlas;


static const uint sample_count = 16;
static const float max_dist = 1.0;



float3 compute_sample_dir(float3 normal, uint i) {
    const float3 s = sample_dirs[i % SAMPLE_BUFFER_SIZE].xyz;
    return dot(normal, s) < 0.0 ? -s : s;
}


[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy * 2;
    const uint2 size = image_size(in_depth);

    if(any(coord >= size)) {
        return;
    }

    const float depth = in_depth[coord];

    float ao = 0;
    if(!is_depth_far(depth)) {
        const float2 uv = (coord + 0.5) / float2(size);
        const float3 world_pos = unproject(uv, depth, camera.inv_view_proj);
        const float3 normal = decode_gbuffer({float4(0.0), in_rt1[coord]}).normal;
        const float view_dist = length(world_pos - camera.position);
        const float min_dist = lerp(0.001, 0.01, view_dist / (view_dist + 10.0));

        const uint sample_index_offset = hash(semantics.global_id.xy);

        for(uint i = 0; i != sample_count; ++i) {
            const float3 sample_dir = compute_sample_dir(normal, i + sample_index_offset);

            const float t = trace_first_hit(tlas, world_pos, sample_dir, min_dist, max_dist);
            ao += sqr(t / max_dist);
        }

        ao /= sample_count;
    }

    out_ao[semantics.global_id.xy] = ao;
}
