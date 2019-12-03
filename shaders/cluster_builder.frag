#version 450

#include "clustered.glsl"

layout(location = 0) out float out_color;

layout(location = 0) flat in uint in_instance_id;
layout(location = 1) in vec2 in_z_range;

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
	const uint min_cluster_z = cluster_z_index(in_z_range.x, data);
	const uint max_cluster_z = cluster_z_index(in_z_range.y, data);

	for(uint cluster_z = min_cluster_z; cluster_z <= max_cluster_z; ++cluster_z) {
		const uint cluster_index =
		    tile_id.x +
		    tile_id.y * data.cluster_count.x +
		    cluster_z * data.cluster_count.x * data.cluster_count.y;

		const uint cluster_light_index = imageAtomicAdd(out_clusters, cluster_coord(tile_id, cluster_z, data), 1);

		const uint cluster_start_index = cluster_index * max_lights_per_cluster;
		if(cluster_light_index < max_lights_per_cluster) {
			const uint index = cluster_start_index + cluster_light_index;
			light_indexes[index] = in_instance_id;
		}
	}

	// This is for debugging
	out_color = 1.0 / float(max_lights_per_cluster);
}
