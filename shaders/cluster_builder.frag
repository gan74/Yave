#version 450

#include "clustered.glsl"

layout(location = 0) out float out_color;

layout(location = 0) flat in uint in_instance_id;

layout(set = 0, binding = 0) uniform Buffer {
	ClusteringData data;
};

layout(set = 0, binding = 1) readonly buffer Lights {
	PointLight lights[];
};

layout(set = 0, binding = 2) writeonly buffer LightIndexes {
	uint light_indexes[];
};

layout(r32ui, set = 0, binding = 3) uniform uimage2D out_clusters;


void main() {
	const uvec2 tile_id = uvec2(gl_FragCoord.xy);
	const uint tile_index = tile_id.x + tile_id.y * data.tile_count.x;
	const uint tile_start_index = tile_index * max_lights_per_cluster;

	const uint tile_light_index = imageAtomicAdd(out_clusters, ivec2(tile_id), 1);
	if(tile_light_index < max_lights_per_cluster) {
		const uint index = tile_start_index + tile_light_index;
		light_indexes[index] = in_instance_id;
	}

	out_color = 1.0 / max_lights_per_cluster;
}
