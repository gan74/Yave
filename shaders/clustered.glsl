#ifndef CLUSTERED_GLSL
#define CLUSTERED_GLSL

#include "yave.glsl"

const uint tile_size = 32;
const uint max_lights_per_cluster = 128;

struct ClusteringData {
	LightingCamera camera;
	mat4 view_proj;
	uvec2 tile_count;
};

#endif

