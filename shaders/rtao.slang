#include "lib/utils.slang"


// -------------------------------- DEFINES --------------------------------

#define SAMPLE_BUFFER_SIZE 256

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
RWTexture2D<float> out_ao;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
cbuffer SampleDirData {
    float4 sample_dirs[SAMPLE_BUFFER_SIZE];
};

[[vk::binding(5)]]
cbuffer Params_Inline {
    uint sample_count;
    float max_dist;
};

[[vk::binding(6)]]
RaytracingAccelerationStructure tlas;


float3 compute_sample_dir(float3 normal, uint i) {
    const float3 s = sample_dirs[i % SAMPLE_BUFFER_SIZE].xyz;
    return dot(normal, s) < 0.0 ? -s : s;
}


float trace_first_hit(RaytracingAccelerationStructure accel, float3 origin, float3 dir, float min_dist, float max_dist) {
    RayDesc ray;
    {
        ray.Origin = origin;
        ray.Direction = dir;
        ray.TMin = min_dist;
        ray.TMax = max_dist;
    }

    RayQuery<RAY_FLAG_FORCE_OPAQUE> query;
    query.TraceRayInline(accel, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

    query.Proceed();

    if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        return query.CommittedRayT();
    }

    return max_dist;
}

[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(in_depth);

    if(any(coord >= size)) {
        return;
    }

    const float depth = in_depth[coord];

    float ao = 0;
    if(!is_depth_far(depth)) {
        const float2 uv = (coord + 0.5) / float2(size);
        const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
        const float3 normal = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[coord])).normal;
        const float view_dist = length(world_pos - camera.position);
        const float min_dist = lerp(0.001, 0.01, view_dist / (view_dist + 10.0));

        const uint sample_index_offset = hash(semantics.global_id.xy);

        for(uint i = 0; i != sample_count; ++i) {
            const float3 sample_dir = compute_sample_dir(normal, i * 7 + sample_index_offset);

            const float t = trace_first_hit(tlas, world_pos, sample_dir, min_dist, max_dist);
            ao += sqr(t / max_dist);
        }

        ao /= sample_count;
    }

    out_ao[coord] = ao;
}
