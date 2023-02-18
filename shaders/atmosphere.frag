#version 450

#include "lib/atmosphere.glsl"

#define USE_LUT

// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_color;
layout(set = 0, binding = 2) uniform sampler2D in_lut;

layout(set = 0, binding = 3) uniform CameraData {
    Camera camera;
};

layout(set = 0, binding = 4) uniform AtmosphereData_Inline {
    AtmosphereParams atmos;
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;



const float km = 1000.0;
const uint step_count = 8;


float optical_depth(vec3 orig, vec3 dir) {
#ifdef USE_LUT
    // UV = (cos theta, altitude)
    const float cos_theta = dot(dir, vec3(0.0, 0.0, 1.0));
    const float normalized_altitude = saturate((length(orig - atmos.center) - atmos.planet_radius) / atmos.atmosphere_height);
    const float depth = textureLod(in_lut, vec2(cos_theta, normalized_altitude), 0.0).x;
    return depth;
#else
    return optical_depth(atmos, orig, dir, atmos.radius * 2.0, step_count * 4);
#endif
}

float optical_depth(vec3 orig, vec3 dir, float len) {
#ifdef USE_LUT
    const float near = optical_depth(orig, dir);
    const float far = optical_depth(orig + dir * len, dir);
    return max(0.0, far - near);
#else
    return optical_depth(atmos, orig, dir, len, step_count);
#endif
}

vec3 compute_scattered_light(vec3 orig, vec3 dir, float len, vec3 bg_color) {
    const float step_size = len / (step_count - 1);
    const vec3 step = dir * step_size;

    float ray_optical_depth = 0.0;
    vec3 pos = orig;
    vec3 in_scattered = vec3(0.0);
    for(uint i = 0; i != step_count; ++i) {
        const float sun_optical_depth = optical_depth(pos, atmos.sun_dir);
        ray_optical_depth = optical_depth(pos, -dir, step_size * i);

        const vec3 transmittance = exp(atmos.scattering_coeffs * -(sun_optical_depth + ray_optical_depth));
        in_scattered += atmosphere_density(atmos, pos) * transmittance * atmos.scattering_coeffs * step_size;
        pos += step;
    }

    return in_scattered + bg_color * exp(-ray_optical_depth);
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

    const vec2 intersection = intersect_sphere(atmos.center, atmos.radius, cam_pos, view_dir);
    const float near = intersection.x;

    const vec4 color = texelFetch(in_color, coord, 0);

    // ray never intersects, we are outside the atmosphere
    if(near < 0.0 || near > view_dist) {
        out_color = color;
    } else {
        const float dist = min(intersection.y, view_dist) - near;
        const vec3 light = compute_scattered_light(cam_pos + view_dir * near, view_dir, dist, color.rgb);
        out_color = vec4(light, color.a);
    }
}

