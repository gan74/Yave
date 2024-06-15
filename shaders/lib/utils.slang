
struct SurfaceInfo {
    float3 albedo;
    float perceptual_roughness;

    float metallic;
    float3 normal;
};


struct RawGBuffer {
    float4 rt0;
    float4 rt1;
}

struct TransformableData {
    float4x4 current;
    float4x4 last;
};

struct Camera {
    float4x4 view_proj;
    float4x4 inv_view_proj;

    float4x4 unjittered_view_proj;
    float4x4 inv_unjittered_view_proj;
    float4x4 prev_unjittered_view_proj;

    float4x4 proj;
    float4x4 inv_proj;

    float4x4 view;
    float4x4 inv_view;

    float3 position;
    uint padding_0;

    float3 forward;
    uint padding_1;

    float3 up;
    uint padding_2;
};

struct MaterialData {
    float3 emissive_factor;
    float roughness_factor;

    float3 base_color_factor;
    float metallic_factor;

    float3 specular_color_factor;
    float specular_factor;

    uint albedo_texture_index;
    uint normal_texture_index;
    uint emissive_texture_index;
    uint metallic_roughness_texture_index;

    uint4 padding;
};









float4 pack_gbuffer_color(float3 color, float metallic) {
    return float4(color, metallic);
}

float4 pack_gbuffer_normal(float3 normal, float roughness) {
    return float4(octahedron_encode(normal), roughness, 0.0);
}


RawGBuffer encode_gbuffer(SurfaceInfo info) {
    RawGBuffer raw;

    raw.rt0 = pack_gbuffer_color(info.albedo, info.metallic);
    raw.rt1 = pack_gbuffer_normal(info.normal, info.perceptual_roughness);

    return raw;
}



// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
float2 octahedron_wrap(float2 v) {
    return (float2(1.0) - abs(v.yx)) * float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

float2 octahedron_encode(float3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : octahedron_wrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}

float3 octahedron_decode(float2 f) {
    f = f * 2.0 - 1.0;
    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 n = float3(f.xy, 1.0 - abs(f.x) - abs(f.y));
    const float t = saturate(-n.z);
    n.xy += float2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
    return normalize(n);
}


float3 unpack_normal_map(float2 normal) {
    normal = normal * 2.0 - 1.0;
    return float3(normal, 1.0 - sqrt(dot(normal, normal)));
}


float4 unpack_2_10_10_10(uint packed) {
    return float4(
        (float3(
            (packed >> 20) & 0x03FF,
            (packed >> 10) & 0x03FF,
            (packed >>  0) & 0x03FF
        ) / float(0x03FF)) * 2.0 - 1.0,
        (packed >> 30 == 0) ? 1.0 : -1.0
    );
}

float4 unpack_color(uint packed) {
    return float4(
        (packed >> 0) & 0xFF,
        (packed >> 8) & 0xFF,
        (packed >> 16) & 0xFF,
        (packed >> 24) & 0xFF
    ) / 255.0;
}
