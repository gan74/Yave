#version 450

#include "lib/atmosphere.glsl"

//#define USE_LUT

// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_color;
layout(set = 0, binding = 2) uniform sampler3D in_lut;

layout(set = 0, binding = 3) uniform CameraData {
    Camera camera;
};

layout(set = 0, binding = 4) uniform AtmosphereData_Inline {
    AtmosphereParams atmos;
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


const float km = 1000.0;

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const float depth = texelFetch(in_depth, coord, 0).x;

    const vec3 view_vec = unproject(in_uv, max(epsilon, depth), camera.inv_view_proj) - camera.position;
    float view_dist = length(view_vec);
    const vec3 view_dir = view_vec / view_dist;


    const float w = lut_w(atmos, view_dist / km);
    const vec4 lut = texture(in_lut, vec3(in_uv, w));

    const vec3 transmittance = exp(atmos.scattering_coeffs * -lut.w);

    const vec4 color = texelFetch(in_color, coord, 0);
    out_color = vec4(color.rgb * transmittance + lut.rgb, color.a);
}

