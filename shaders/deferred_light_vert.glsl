#include "deferred_light.glsl"

layout(set = 0, binding = 0) uniform CameraData {
    Camera camera;
};

layout(set = 0, binding = 1) readonly buffer Lights {
    Light lights[];
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) flat out uint out_instance_id;

void main() {
    out_instance_id = gl_InstanceIndex;
    Light light = lights[gl_InstanceIndex];

    gl_Position = camera.view_proj * vec4(in_position * light.radius + light.position, 1.0);
}

