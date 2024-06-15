import "glsl";

#include "lib/utils.slang"



struct VertexStageIn {
    float3 position;
    uint2 packed_normal_tangent_sign;
    float2 uv;
};


struct PixelStageIn {
    float3 normal;
    float2 uv;

    float3 tangent;
    float3 bitangent;

    float3 screen_pos;
    float3 last_screen_pos;

    uint instance_index;
};

struct VertexStageOut {
    float4 sv_position  : SV_Position;
    PixelStageIn vert;
};

struct PixelStageOut {
    float2 motion;
    RawGBuffer gbuffer;
    float3 emissive;
};





[[vk::binding(0, 0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(0, 1)]]
StructuredBuffer<TransformableData> transformables;

[[vk::binding(1, 1)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(2, 1)]]
StructuredBuffer<uint2> transform_material_indices;

[[vk::binding(0, 2)]]
Sampler2D all_textures[];




[shader("vertex")]
VertexStageOut vert_main(VertexStageIn in) {
    const TransformableData transformable = transformables[transform_material_indices[gl_InstanceIndex].x];

    const float3x3 model = float3x3(transformable.current);

    const float4 current_position = mul(camera.unjittered_view_proj, mul(transformable.current, float4(in.position, 1.0)));
    const float4 last_position = mul(camera.prev_unjittered_view_proj, mul(transformable.last, float4(in.position, 1.0)));

    const float3 in_normal = unpack_2_10_10_10(in.packed_normal_tangent_sign.x).xyz;
    const float4 in_tangent_sign = unpack_2_10_10_10(in.packed_normal_tangent_sign.y);

    VertexStageOut out;
    {
        out.sv_position = mul(camera.view_proj, mul(transformable.current, float4(in.position, 1.0)));

        out.vert.normal = normalize(mul(model, in_normal));
        out.vert.uv = in.uv;
        out.vert.tangent = normalize(mul(model, in_tangent_sign.xyz));
        out.vert.bitangent = cross(out.vert.tangent, out.vert.normal) * in_tangent_sign.w;
        out.vert.screen_pos = current_position.xyw;
        out.vert.last_screen_pos = last_position.xyw;
        out.vert.instance_index = gl_InstanceIndex;
    }
    return out;
}





[shader("fragment")]
PixelStageOut frag_main(PixelStageIn in) {
    const MaterialData material = materials[transform_material_indices[in.instance_index].y];

    const Sampler2D albedo_tex = all_textures[material.albedo_texture_index];
    const Sampler2D normal_tex = all_textures[material.normal_texture_index];
    const Sampler2D emissive_tex = all_textures[material.emissive_texture_index];
    const Sampler2D metal_rough_tex = all_textures[material.metallic_roughness_texture_index];

    const float3 normal = unpack_normal_map(normal_tex.Sample(in.uv).xy);
    const float3 mapped_front_face_normal = normal.x * in.tangent + normal.y * in.bitangent + normal.z * in.normal;
    const float3 mapped_normal = gl_FrontFacing ? mapped_front_face_normal : -mapped_front_face_normal;

    const float4 albedo = albedo_tex.Sample(in.uv);

#ifdef ALPHA_TEST
    if(albedo.a < 0.5) {
        discard;
    }
#endif

    const float4 metallic_roughness = metal_rough_tex.Sample(in.uv);

    SurfaceInfo surface;
    {
        surface.albedo = albedo.rgb * material.base_color_factor;
        surface.normal = mapped_normal;

        surface.perceptual_roughness = metallic_roughness.g * material.roughness_factor;
        surface.metallic = metallic_roughness.b * material.metallic_factor;
    }


    PixelStageOut out;
    out.gbuffer = encode_gbuffer(surface);
    out.motion = ((in.last_screen_pos.xy / in.last_screen_pos.z) - (in.screen_pos.xy / in.screen_pos.z)) * 0.5;
    out.emissive = emissive_tex.Sample(in.uv).rgb * material.emissive_factor;
    return out;
}
