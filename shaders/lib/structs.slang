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

#endif
