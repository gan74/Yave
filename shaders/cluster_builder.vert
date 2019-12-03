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
layout(location = 1) out vec2 out_z_range;


void main() {
	out_instance_id = gl_InstanceIndex;

	const PointLight light = lights[gl_InstanceIndex];

	const vec3 norm = /*normalize*/(in_normal - dot(data.camera.forward, in_normal) * data.camera.forward);
	const vec3 pos = light.position + in_position * light.radius;

	const vec3 proj_pos = project(pos, data.view_proj);
	const vec3 proj_pos_norm = project(pos + norm, data.view_proj);
	const float dist = distance(proj_pos.xy, proj_pos_norm.xy);

	const float sqrt_2 = 1.41421356237;
	const float target = length(1.0 / data.cluster_count.xy) / sqrt_2;
	const float norm_len = target / dist;

	const vec3 out_position = light.position + in_position * light.radius + norm * norm_len;
	gl_Position = data.view_proj * vec4(out_position, 1.0);

	const float max_z = dot(out_position - data.camera.position, data.camera.forward);
	const float min_z = max_z - 2.0 * light.radius;
	out_z_range = vec2(min_z, max_z);
}
