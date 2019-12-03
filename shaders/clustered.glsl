#ifndef CLUSTERED_GLSL
#define CLUSTERED_GLSL

#include "yave.glsl"

const uint tile_size = 32;
const uint max_lights_per_cluster = 128;

struct ClusteringData {
	LightingCamera camera;
	mat4 view_proj;
	uvec3 cluster_count;
};

uint cluster_z_index(float distance, ClusteringData data) {
	/*const float ratio = data.z_far / data.z_near;
	const float z_ratio = (distance + data.z_near)  / data.z_near;
	const uint index = uint(data.cluster_count.z * log(z_ratio) / log(ratio));*/
	const uint index = uint(1.23315 * log(0.25 * max(4.0, distance)));
	return clamp(index, 0, data.cluster_count.z - 1);
}

ivec2 cluster_coord(uvec2 tile_id, uint cluster_z, ClusteringData data) {
	return ivec2(tile_id + uvec2(0, cluster_z * data.cluster_count.y));
}

#endif

