#version 450

#include "clustered.glsl"

// -------------------------------- I/O --------------------------------

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec2 in_uv;

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_color;
layout(set = 0, binding = 2) uniform sampler2D in_normal;

layout(set = 0, binding = 3) uniform usampler2D in_clusters;

layout(set = 0, binding = 4) readonly buffer Lights {
	PointLight lights[];
};

layout(set = 0, binding = 5) readonly buffer LightIndexes {
	uint light_indexes[];
};

layout(set = 0, binding = 6) uniform Buffer {
	ClusteringData data;
};


// -------------------------------- MAIN --------------------------------

void main() {
	const ivec2 coord = ivec2(gl_FragCoord.xy);
	const uvec2 tile_id = uvec2(coord) / tile_size;

	const float depth = texelFetch(in_depth, coord, 0).x;
	vec3 irradiance = vec3(0.0);

	if(!is_OOB(depth)) {
		vec3 albedo;
		float metallic;
		vec3 normal;
		float roughness;
		unpack_color(texelFetch(in_color, coord, 0), albedo, metallic);
		unpack_normal(texelFetch(in_normal, coord, 0), normal, roughness);

		const vec3 world_pos = unproject(in_uv, depth, data.camera.inv_matrix);
		const vec3 view_vec = data.camera.position - world_pos;
		const float view_z = -dot(data.camera.forward, view_vec);
		const uint cluster_z = cluster_z_index(view_z, data);
		const vec3 view_dir = normalize(view_vec);

		const uint cluster_index =
		    tile_id.x +
		    tile_id.y * data.cluster_count.x +
		    cluster_z * data.cluster_count.x * data.cluster_count.y;

		const uint cluster_start_index = cluster_index * max_lights_per_cluster;

		const uint light_count = min(max_lights_per_cluster, texelFetch(in_clusters, cluster_coord(tile_id, cluster_z, data), 0).x);
		for(uint i = 0; i != light_count; ++i) {
			const uint light_index = light_indexes[cluster_start_index + i];
			const PointLight light = lights[light_index];

			// light_dir dot view_dir > 0
			vec3 light_dir = light.position - world_pos;
			const float distance = length(light_dir);
			light_dir /= distance;
			const float att = attenuation(distance, light.radius, light.falloff);

			const vec3 radiance = light.color * att;
			irradiance += radiance * L0(normal, light_dir, view_dir, roughness, metallic, albedo);
		}
	}

	out_color = vec4(irradiance, 1.0f);
}


