#version 450

#include "clustered.glsl"

layout(set = 0, binding = 0) uniform Buffer {
	ClusteringData data;
};

layout(set = 0, binding = 1) readonly buffer Lights {
	PointLight lights[];
};


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out uint out_instance_id;

void main() {
	out_instance_id = gl_InstanceIndex;

	const PointLight light = lights[gl_InstanceIndex];

	gl_Position = data.view_proj * vec4(light.position + in_position * light.radius, 1.0);
}
