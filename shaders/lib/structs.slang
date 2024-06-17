#ifndef STRUCT_SLANG
#define STRUCT_SLANG

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

struct ExposureParams {
    float exposure;
    float avg_lum;
    float max_lum;

    uint padding_0;
};

struct DirectionalLight {
    float3 direction;
    float cos_disk;

    float3 color;
    uint padding_1;

    uint4 shadow_map_indices;
};

struct ShadowMapInfo {
    float4x4 view_proj;

    float2 uv_offset;
    float2 uv_mul;

    float size;
    float texel_size;
    uint padding_0;
    uint padding_1;
};

struct AtmosphereParams {
    float3 center;
    float planet_radius;

    float3 scattering_coeffs;
    float atmosphere_height;

    float3 sun_dir;
    float radius; // planet_radius + atmosphere_height

    float3 sun_color;
    float density_falloff;
};

#endif
