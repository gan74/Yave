#ifndef RT_SLANG
#define RT_SLANG

float trace_first_hit(RaytracingAccelerationStructure accel, float3 origin, float3 dir, float min_dist, float max_dist) {
    RayDesc ray;
    {
        ray.Origin = origin;
        ray.Direction = dir;
        ray.TMin = min_dist;
        ray.TMax = max_dist;
    }

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    query.TraceRayInline(accel, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, ray);

    query.Proceed();

    if(query.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
        return query.CommittedRayT();
    }

    return max_dist;
}

#endif // RT_SLANG
