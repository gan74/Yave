#version 450

#include "lib/atmosphere.glsl"

// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_lut;

layout(set = 0, binding = 2) uniform CameraData {
    Camera camera;
};

layout(set = 0, binding = 3) uniform AtmosphereData {
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


const uint sample_count = 16;
const float km = 1000.0;


// -------------------------------- HELPERS --------------------------------

float atmosphere_density(float normalized_altitude) {
    return exp(-normalized_altitude * density_falloff) * (1.0 - normalized_altitude);
}

float atmosphere_density(vec3 pos) {
    const float altitude = length(center - pos) - planet_radius;
    const float normalized_alt = altitude / (radius - planet_radius);
    return atmosphere_density(normalized_alt);
}

float optical_depth(vec3 orig, vec3 dir, float len) {
    const float step_size = len / (sample_count - 1);
    const vec3 step = dir * step_size;

    float depth = 0.0;
    vec3 pos = orig;
    for(uint i = 0; i != sample_count; ++i) {
        const float density = atmosphere_density(pos);
        depth += density * step_size;
        pos += step;
    }

    return depth;
}

vec3 compute_light(vec3 orig, vec3 dir, float len) {
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
        const vec2 ground = intersect_sphere(center, planet_radius, cam_pos, view_dir);
        if(ground.x > 0.0) {
            view_dist = min(view_dist, ground.x);
        }
    }

    const vec2 intersection = intersect_sphere(center, radius, cam_pos, view_dir);

    vec3 light = vec3(0.0);
    if(intersection.x >= 0.0 && intersection.x < view_dist) {
        const vec3 start = cam_pos + view_dir * intersection.x;

        const float dist = min(intersection.y, view_dist - intersection.x);

        light = compute_light(start, view_dir, dist) * light_color;
    }

    out_color = vec4(light, 1.0);
}
