import "glsl";

#include "lib/utils.slang"

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

    uint texture_indices[8];
};



struct InOutVertex {

};

struct VertexStageIn {
    float3 position;
    uint2 packed_normal_tangent_sign;
    float2 uv;
};

struct VertexStageOut {
    float4 position  : SV_Position;
    float3 normal;
    float2 uv;

    float3 tangent;
    float3 bitangent;

    float3 screen_pos;
    float3 last_screen_pos;

    uint instance_index;
};




[[vk::binding(0, 0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(0, 1)]]
StructuredBuffer<TransformableData> transformables;

[[vk::binding(1, 1)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(2, 1)]]
StructuredBuffer<uint2> transform_material_indices;

[shader("vertex")]
VertexStageOut vert_main(VertexStageIn in) {
    const TransformableData transformable = transformables[transform_material_indices[gl_InstanceIndex].x];

    const float3x3 model = float3x3(transformable.current);

    const float4 current_position = (camera.unjittered_view_proj * transformable.current * float4(in.position, 1.0));
    const float4 last_position = (camera.prev_unjittered_view_proj * transformable.last * float4(in.position, 1.0));

    const float3 in_normal = unpack_2_10_10_10(in.packed_normal_tangent_sign.x).xyz;
    const float4 in_tangent_sign = unpack_2_10_10_10(in.packed_normal_tangent_sign.y);

    VertexStageOut out;
    {
        out.position = camera.view_proj * transformable.current * float4(in.position, 1.0);
        out.normal = normalize(model * in_normal);
        out.uv = in.uv;

        out.tangent = normalize(model * in_tangent_sign.xyz);
        out.bitangent = cross(out.tangent, out.normal) * in_tangent_sign.w;

        out.screen_pos = current_position.xyw;
        out.last_screen_pos = last_position.xyw;

        out.instance_index = gl_InstanceIndex;
    }
    return out;
}

[shader("fragment")]
float4 frag_main(VertexStageOut in : VertexStageOut) : SV_Target {
    return float4(in.uv, 0.0, 1.0);
}
