#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "brdf.glsl"
#include "gbuffer.glsl"





struct AreaLightInfo {
    vec3 light_dir;
    float orig_light_dist;
    float light_dist;
    float sqr_alpha;
};

// https://alextardif.com/arealights.html
AreaLightInfo karis_area_light(SurfaceInfo surface, vec3 light_pos, float source_radius, vec3 world_pos, vec3 view_dir) {
    const vec3 refl = reflect(-view_dir, surface.normal);
    const vec3 to_light = light_pos - world_pos;
    const vec3 center = (dot(to_light, refl) * refl) - to_light;
    const vec3 closest_point = to_light + center * saturate(source_radius / length(center));

    AreaLightInfo info;
    info.light_dir = normalize(closest_point);
    info.orig_light_dist = length(to_light);
    info.light_dist = length(closest_point);
    info.sqr_alpha = saturate(source_radius / (info.light_dist * 2.0) + surface.alpha) * surface.alpha;
    return info;
}

AreaLightInfo karis_area_light(SurfaceInfo surface, PointLight light, vec3 world_pos, vec3 view_dir) {
    return karis_area_light(surface, light.position, light.min_radius, world_pos, view_dir);
}

AreaLightInfo karis_area_light(SurfaceInfo surface, SpotLight light, vec3 world_pos, vec3 view_dir) {
    return karis_area_light(surface, light.position, light.min_radius, world_pos, view_dir);
}

SurfaceInfo alpha_corrected(SurfaceInfo surface, AreaLightInfo area) {
    surface.sqr_alpha = area.sqr_alpha;
    return surface;
}



float attenuation(float distance, float range, float light_radius) {
    const float num = max(0.0, 1.0 - sqr(sqr(distance / range)));
    return sqr(num / max(distance, light_radius));
}

float spot_attenuation(float cos_angle, vec2 scale_offset) {
    const float att = saturate(cos_angle * scale_offset.x + scale_offset.y);
    return sqr(att);
}



vec3 L0(vec3 light_dir, vec3 view_dir, SurfaceInfo surface) {
    const vec3 half_vec = normalize(light_dir + view_dir);

    const float NoV = max(0.0, dot(surface.normal, view_dir));
    const float NoL = max(0.0, dot(surface.normal, light_dir));
    const float NoH = max(0.0, dot(surface.normal, half_vec));
    const float VoH = max(0.0, dot(view_dir, half_vec));

    const vec3  F = F_Schlick(VoH, surface.F0);
    const float D = D_GGX(NoH, surface.sqr_alpha);
    const float V = V_Smith(NoV, NoL, surface.sqr_alpha);

    const vec3 kS = F;
    const vec3 kD = (1.0 - kS) * (1.0 - surface.metallic);

    const vec3 specular = kS * max(0.0, D * V);
    const vec3 diffuse = kD * Lambert_diffuse_brdf(surface);

    return (diffuse + specular) * NoL;
}

#endif // LIGHTING_GLSL

