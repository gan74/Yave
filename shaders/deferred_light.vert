#version 450

#include "lib/utils.glsl"

// -------------------------------- DEFINES --------------------------------

#ifdef POINT
#define Light PointLight
#endif

#ifdef SPOT
#define Light SpotLight
#endif

layout(set = 0, binding = 0) uniform CameraData {
    Camera camera;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in uvec2 in_packed_normal_tangent_sign;
layout(location = 2) in vec2 in_uv;

#ifdef SPOT_LIGHT
layout(location = 8) in mat4 in_spot;
#else
layout(set = 0, binding = 1) readonly buffer Lights {
    Light lights[];
};
#endif

layout(location = 0) flat out uint out_instance_id;

void main() {
    out_instance_id = gl_InstanceIndex;

#ifdef SPOT_LIGHT
    gl_Position = camera.view_proj * in_spot * vec4(in_position, 1.0);
#else
    Light light = lights[gl_InstanceIndex];
    const float scale = light.range * 1.1;
    gl_Position = camera.view_proj * vec4(in_position * scale + light.position, 1.0);
#endif

}

