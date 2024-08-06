import "glsl";

#include "lib/utils.slang"


// -------------------------------- DEFINES --------------------------------

#define SAMPLE_BUFFER_SIZE 256

// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
RWTexture2D<float> out_ao;

[[vk::binding(1)]]
Sampler2D in_depth;

[[vk::binding(2)]]
Sampler2D in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
cbuffer SampleDirData {
    vec4 sample_dirs[SAMPLE_BUFFER_SIZE];
};

[[vk::binding(5)]]
RaytracingAccelerationStructure tlas;


static const uint sample_count = 64;
static const float max_dist = 1.0;




uint hash_1_2(uvec2 x) {
    uvec2 q = 1103515245U * ((x >> 1U) ^ x.yx);
    uint n = 1103515245U * (q.x ^ (q.y >> 3U));
    return n;
}

vec3 compute_sample_dir(vec3 normal, uint i) {
    const vec3 s = sample_dirs[i % SAMPLE_BUFFER_SIZE].xyz;
    return dot(normal, s) < 0.0 ? -s : s;
}



float trace_first_hit(RaytracingAccelerationStructure accel, float3 origin, float3 dir, float max_dist) {
    RayDesc ray;
    {
        ray.Origin = origin;
        ray.TMin = 0.01;
        ray.Direction = dir;
        ray.TMax = max_dist;
    }

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    query.TraceRayInline(accel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, ray);

    query.Proceed();

    if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        return query.CommittedRayT();
    }

    return -1.0;
}


[shader("compute")]
[numthreads(8, 8)]
void main() {
    const ivec2 coord = ivec2(gl_GlobalInvocationID.xy * 2);
    const ivec2 image_size = textureSize(in_depth, 0).xy;

    if(any(greaterThan(coord, image_size))) {
        return;
    }

    const float depth = texelFetch(in_depth, coord, 0).x;

    float ao = 0;
    if(!is_depth_far(depth)) {
        const vec2 uv = (coord + 0.5) / vec2(image_size);
        const vec3 world_pos = unproject(uv, depth, camera.inv_view_proj);
        const vec3 normal = decode_gbuffer({vec4(0.0), texelFetch(in_rt1, coord, 0)}).normal;

        const uint sample_index_offset = hash_1_2(gl_GlobalInvocationID.xy);

        for(uint i = 0; i != sample_count; ++i) {
            const vec3 sample_dir = compute_sample_dir(normal, i + sample_index_offset);

            const float t = trace_first_hit(tlas, world_pos, sample_dir, max_dist);

            if(t >= 0) {
                ao += 1.0;
            }
        }

        ao = 1.0 - sqr(ao / sample_count);
    }

    out_ao[ivec2(gl_GlobalInvocationID.xy)] =  ao;
}
