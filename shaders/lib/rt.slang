#ifndef RT_SLANG
#define RT_SLANG

#include "utils.slang"

struct HitInfo {
    bool hit;
    bool front_face;
    float ray_t;
    float3 position;
    uint instance_id;
    uint primitive_id;
    float2 barycentrics;
    float3 ms_vertices[3];
    float3 ms_normal;
    float3x4 model_to_world;


    __init() {
        hit = false;
        front_face = true;
        ray_t = -1.0;
        position = float3(0.0);
        instance_id = primitive_id = 0;
        barycentrics = float2(0.0);
        ms_vertices = {float3(0.0), float3(0.0), float3(0.0)};
        ms_normal = float3(0.0);
        model_to_world = float3x4(0.0);
    }
};

HitInfo trace_inline(RaytracingAccelerationStructure accel, float3 origin, float3 dir, float min_dist, float max_dist) {
    RayDesc ray;
    {
        ray.Origin = origin;
        ray.Direction = dir;
        ray.TMin = min_dist;
        ray.TMax = max_dist;
    }

    RayQuery<RAY_FLAG_FORCE_OPAQUE> query;
    query.TraceRayInline(accel, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

    /*while*/ (query.Proceed());

    HitInfo info;
    if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        info.hit = true;
        info.front_face = query.CommittedRayFrontFace();
        info.ray_t = query.CommittedRayT();
        info.instance_id = query.CommittedInstanceID();
        info.primitive_id = query.CommittedPrimitiveIndex();
        info.barycentrics = query.CommittedTriangleBarycentrics();
        info.ms_vertices = query.CommittedGetIntersectionTriangleVertexPositions();
        info.model_to_world = query.CommittedObjectToWorld3x4();

        info.position = origin + dir * info.ray_t;
        info.ms_normal = normalize(cross((info.ms_vertices[0] - info.ms_vertices[1]), (info.ms_vertices[0] - info.ms_vertices[2])));
    } else {
        info.ray_t = max_dist;
    }

    return info;
}


#endif // RT_SLANG
