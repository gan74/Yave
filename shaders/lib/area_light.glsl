#ifndef AREA_LIGHT_GLSL
#define AREA_LIGHT_GLSL

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

SurfaceInfo alpha_corrected_surface(SurfaceInfo surface, AreaLightInfo area) {
    surface.sqr_alpha = area.sqr_alpha;
    return surface;
}


#endif // AREA_LIGHT_GLSL

