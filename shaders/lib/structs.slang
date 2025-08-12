#ifndef STRUCT_SLANG
#define STRUCT_SLANG


enum TextureSlots : uint32_t {
    Albedo                      = 0,
    Normal                      = 1,
    Emissive                    = 2,
    MetallicRoughnessSpecular   = 3,
    SpecularColor               = 4,
};

enum TAAFeatureBits : uint32_t {
    ClampingBit                 = 0x01,
    DenoiseBit                  = 0x02,
};

enum TAAMask : uint32_t {
    None                        = 0,
    UvOutOfBounds               = 1,
    MotionMismatch              = 2,
    DepthOutOfBounds            = 3,
    Deoccluded                  = 4,
};


struct TransformableData {
    float4x4 current;
    float4x4 last;
};

struct CameraMatrices {
    float4x4 view_proj;
    float4x4 inv_view_proj;
};

struct Camera {
    CameraMatrices curr;
    CameraMatrices prev;

    float2 jitter;
    float2 prev_jitter;

    float2 combined_jitter;
    float2 padding;

    float4x4 proj;
    float4x4 inv_proj;

    float4x4 view;
    float4x4 inv_view;

    float3 position;
    float aspect;

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

    uint texture_indices[8];
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

struct PointLight {
    float3 position;
    float range;

    float3 color;
    float falloff;

    float3 padding_0;
    float min_radius;
};

struct SpotLight {
    float3 position;
    float range;

    float3 color;
    float falloff;

    float3 forward;
    float min_radius;

    float2 att_scale_offset;
    float sin_angle;
    uint shadow_map_index;

    float3 encl_sphere_center;
    float encl_sphere_radius;

    float4x4 draw_model; // TODO remove
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

struct SHBase {
    float Y00, Y11, Y10, Y1_1, Y21, Y2_1, Y2_2, Y20, Y22;

#ifndef __cplusplus
    __init() {
        Y00 = Y11 = Y10 = Y1_1 = Y21 = Y2_1 = Y2_2 = Y20 = Y22 = 0.0;
    }
#endif
};

#endif
