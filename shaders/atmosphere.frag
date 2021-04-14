#version 450

#include "lib/utils.glsl"

#define USE_LUT

// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_lut;

layout(set = 0, binding = 2) uniform CameraData {
    Camera camera;
};

layout(set = 0, binding = 3) uniform AtmosphereData_Inline {
    vec3 center;
    float planet_radius;

    vec3 rayleigh;
    float atmosphere_height;

    vec3 light_dir;
    float radius;

    vec3 light_color;
    float density_falloff;
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


const uint optical_depth_sample_count = 16;
const uint sample_count = 8;
const float km = 1000.0;


// -------------------------------- HELPERS --------------------------------

float normalized_altitude(float altitude) {
    return altitude / (radius - planet_radius);
}

float normalized_altitude(vec3 pos) {
    const float altitude = length(center - pos) - planet_radius;
    return normalized_altitude(altitude);
}

float atmosphere_density(float norm_alt) {
    return exp(-norm_alt * density_falloff) * (1.0 - norm_alt);
}

float atmosphere_density(vec3 pos) {
    return atmosphere_density(normalized_altitude(pos));
}


float optical_depth_from_lut(vec3 orig, vec3 dir) {
    const float cos_t = dot(normalize(orig - center), normalize(dir));
    const float norm_alt = normalized_altitude(orig);

    return texture(in_lut, vec2(cos_t * 0.5 + 0.5, norm_alt)).x;
}

float optical_depth_from_lut(vec3 orig, vec3 dir, float len) {
    const vec3 end = orig + dir * len;
    return optical_depth_from_lut(orig, dir) - optical_depth_from_lut(end, dir);
}

float optical_depth(vec3 orig, vec3 dir, float len) {
#ifdef USE_LUT
    return optical_depth_from_lut(orig, dir, len);
#else
    const float step_size = len / (optical_depth_sample_count - 1);
    const vec3 step = dir * step_size;

    float depth = 0.0;
    vec3 pos = orig;
    for(uint i = 0; i != optical_depth_sample_count; ++i) {
        const float density = atmosphere_density(pos);
        depth += density * step_size;
        pos += step;
    }

    return depth;
#endif
}




vec3 compute_scattered_light(vec3 orig, vec3 dir, float len) {
    const vec3 scattering_coef = rayleigh; //pow(vec3(scattering_strength) / wavelengths, vec3(4.0));

    const float step_size = len / (sample_count - 1);
    const vec3 step = dir * step_size;

    vec3 scattered_light = vec3(0.0);
    vec3 pos = orig;
    for(uint i = 0; i != sample_count; ++i) {
        const float light_ray_len = intersect_sphere(center, radius, pos, light_dir).y;
        const float light_optical_depth = optical_depth(pos, light_dir, light_ray_len);
        const float ray_optical_depth = optical_depth(pos, -dir, i * step_size);

        const vec3 transmittance = exp(-(light_optical_depth + ray_optical_depth) * scattering_coef);

        scattered_light += transmittance * scattering_coef * (atmosphere_density(pos) * step_size);
        pos += step;
    }

    return scattered_light;
}


// -------------------------------- MAIN --------------------------------


void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const float depth = texelFetch(in_depth, coord, 0).x;
    const vec3 cam_pos = camera.position / km;

    const vec3 world_pos = unproject(in_uv, max(epsilon, depth), camera.inv_view_proj) / km;
    const vec3 view_vec = world_pos - cam_pos;
    float view_dist = length(view_vec);
    const vec3 view_dir = view_vec / view_dist;

    {
        const float ground_dist = intersect_sphere(center, planet_radius, cam_pos, view_dir).x;
        if(ground_dist > 0.0) {
            view_dist = min(view_dist, ground_dist);
        }
    }

    const vec2 intersection = intersect_sphere(center, radius, cam_pos, view_dir);
    const float enters = intersection.x;
    const float exits = intersection.y;

    vec3 light = vec3(0.0);

    if(enters >= 0.0 && enters < view_dist) {
        const vec3 start = cam_pos + view_dir * enters;
        const float dist = min(exits, view_dist - enters);
        light = compute_scattered_light(start, view_dir, dist) * light_color;
    }

    out_color = vec4(light, 1.0);
}

