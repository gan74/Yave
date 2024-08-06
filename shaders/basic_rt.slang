import "glsl";

#include "lib/utils.slang"

[[vk::binding(0)]]
RaytracingAccelerationStructure  tlas;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
RWTexture2D out_image;



struct RayPayload {
    float3 hit_value;
};




[shader("raygeneration")]
void rgen_main() {
    const float2 px_center = float2(gl_LaunchIDEXT.xy) + float2(0.5);
    const float2 in_uv = px_center / float2(gl_LaunchSizeEXT.xy);

    const float4 origin = camera.inv_view * float4(0.0, 0.0, 0.0, 1.0);
    const float4 target = camera.inv_proj * float4(in_uv * 2.0 - 1.0, 1.0, 1.0);
    const float4 dir = camera.inv_view * float4(normalize(target.xyz), 0.0) ;

    const float tmin = 0.001;
    const float tmax = 10000.0;

    RayDesc ray;
    {
        ray.Origin = origin.xyz;
        ray.Direction = dir.xyz;
        ray.TMin = 0.001;
        ray.TMax = 10000.0;
    }

    RayPayload payload;
    TraceRay(tlas, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);

    out_image[int2(gl_LaunchIDEXT.xy)] = float4(payload.hit_value, 0.0);
}


[shader("miss")]
void miss_main(inout RayPayload payload) {
    payload.hit_value = float3(0.0);
}


[shader("closesthit")]
void rchit_main(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs) {
    const float3 bary = float3(1.0f - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
    payload.hit_value = bary;
}
