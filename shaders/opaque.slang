#include "lib/std_bindings.slang"
#include "lib/utils.slang"



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
    PixelStageIn frag;
};

struct PixelStageOut {
    float2 motion;
    RawGBuffer gbuffer;
    float3 emissive;

    __init() {
        motion = 0.0;
        emissive = 0.0;
    }
};



[[vk::binding(0, 1)]]
Sampler2D all_textures[];


[shader("vertex")]
VertexStageOut vert_main() {
    const MeshObject mesh = mesh_objects[semantics.instance_index];
    const StdVertexData vert = read_vertex(semantics.vertex_index, mesh_datas[mesh.mesh_data_index]);
    const TransformableData transformable = transformables[mesh.transform_index];

    const float3x3 model = float3x3(transformable.current);

    const float4 current_position = mul(camera.curr.view_proj, mul(transformable.current, float4(vert.position, 1.0)));
    const float4 last_position = mul(camera.prev.view_proj, mul(transformable.last, float4(vert.position, 1.0)));

    const float3 in_normal = unpack_2_10_10_10(vert.packed_normal_tangent_sign.x).xyz;
    const float4 in_tangent_sign = unpack_2_10_10_10(vert.packed_normal_tangent_sign.y);

    VertexStageOut out;
    {
        out.sv_position = current_position;

        out.frag.normal = normalize(mul(model, in_normal));
        out.frag.uv = vert.uv;
        out.frag.tangent = normalize(mul(model, in_tangent_sign.xyz));
        out.frag.bitangent = cross(out.frag.tangent, out.frag.normal) * in_tangent_sign.w;
        out.frag.screen_pos = current_position.xyw;
        out.frag.last_screen_pos = last_position.xyw;
        out.frag.instance_index = semantics.instance_index;
    }
    return out;
}





// https://github.com/nvpro-samples/vk_raytrace/blob/master/shaders/gltf_material.glsl
float perceived_brightness(float3 color) {
    return sqrt(dot(float3(0.299, 0.587, 0.114), color * color));
}

float metalic_from_specular(float3 diffuse, float3 specular) {
    const float min_reflectance = 0.04;

    const float reflectance = max(max(specular.r, specular.g), specular.b);
    const float one_minus_spec_strength = saturate(1.0 - reflectance);

    const float specular_brightness = perceived_brightness(specular);
    const float diffuse_brightness = perceived_brightness(diffuse);

    if(specular_brightness < min_reflectance) {
        return 0.0;
    }

    const float a = min_reflectance;
    const float b = diffuse_brightness * one_minus_spec_strength / (1.0 - min_reflectance) + specular_brightness - 2.0 * min_reflectance;
    const float c = min_reflectance - specular_brightness;
    const float D = max(b * b - 4.0 * min_reflectance * c, 0.0);

    return saturate((-b + sqrt(D)) / (2.0 * a));
}



[shader("fragment")]
PixelStageOut frag_main(PixelStageIn in) {
    const MaterialData material = materials[mesh_objects[in.instance_index].material_index];


    const Sampler2D albedo_tex = all_textures[NonUniformResourceIndex(material.texture_indices[uint(TextureSlots::Albedo)])];
    const Sampler2D normal_tex = all_textures[NonUniformResourceIndex(material.texture_indices[uint(TextureSlots::Normal)])];
    const Sampler2D emissive_tex = all_textures[NonUniformResourceIndex(material.texture_indices[uint(TextureSlots::Emissive)])];
    const Sampler2D metal_rough_spec_tex = all_textures[NonUniformResourceIndex(material.texture_indices[uint(TextureSlots::MetallicRoughnessSpecular)])];
    const Sampler2D spec_color_tex = all_textures[NonUniformResourceIndex(material.texture_indices[uint(TextureSlots::SpecularColor)])];

    const float3 normal = unpack_normal_map(normal_tex.Sample(in.uv).xy);
    const float3 mapped_front_face_normal = /*normal.x * in.tangent + normal.y * in.bitangent + normal.z */ in.normal;
    const float3 mapped_normal = semantics.is_front_face ? mapped_front_face_normal : -mapped_front_face_normal;

    const float4 albedo_alpha = albedo_tex.SampleLevel(in.uv, 99999.0);
    const float3 albedo = albedo_alpha.rgb * material.base_color_factor;
    // const float3 albedo = heat_spectrum(frac(mesh_objects[in.instance_index].mesh_data_index / 128.0));

#ifdef ALPHA_TEST
    if(albedo_alpha.a < 0.5) {
        discard;
    }
#endif


#ifdef SPECULAR
    const float gloss = metal_rough_spec_tex.Sample(in.uv).a * material.specular_factor;
    const float3 specular = spec_color_tex.Sample(in.uv).rgb * material.specular_color_factor;
    const float roughness = saturate(1.0 - gloss);
    const float metallic = metalic_from_specular(albedo, specular);
#else
    const float4 metallic_roughness = metal_rough_spec_tex.Sample(in.uv);
    const float roughness = metallic_roughness.g * material.roughness_factor;
    const float metallic = metallic_roughness.b * material.metallic_factor;
#endif

    SurfaceInfo surface;
    {
        surface.albedo = albedo;
        surface.normal = mapped_normal;

        surface.perceptual_roughness = roughness;
        surface.metallic = metallic;
    }


    PixelStageOut out;
    out.gbuffer = encode_gbuffer(surface);
    out.motion = ((in.last_screen_pos.xy / in.last_screen_pos.z) - (in.screen_pos.xy / in.screen_pos.z) - camera.combined_jitter) * 0.5;
    out.emissive = emissive_tex.Sample(in.uv).rgb * material.emissive_factor;
    return out;
}
