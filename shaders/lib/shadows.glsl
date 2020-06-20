#ifndef SHADOWS_GLSL
#define SHADOWS_GLSL

#include "utils.glsl"

float variance_shadow(vec2 moments, float depth) {
	const float variance = sqr(moments.x) - moments.y;
	const float diff = moments.x - depth;
	const float p = variance / (variance + sqr(diff));
	return max(p, depth <= moments.x ? 1.0 : 0.0);
}

vec4 sort_depths(vec4 depths) {
	const float min0 = min(depths.x, depths.y);
	const float min1 = min(depths.z, depths.w);

	const float max0 = max(depths.x, depths.y);
	const float max1 = max(depths.z, depths.w);

	const vec4 min_max = vec4(min(min0, min1), max(min0, min1), min(max0, max1), max(max0, max1));

	return (min_max.y > min_max.z) ? min_max.xzyw : min_max.xyzw;
}

float compute_shadow(sampler2D shadow_map, ShadowMapParams params, vec3 world_pos) {

	const vec3 proj = project(world_pos, params.view_proj);
	const vec2 uvs = params.uv_offset + proj.xy * params.uv_mul;

	const vec4 depths = textureGather(shadow_map, uvs);
	const float min_depth = min(min(depths.x, depths.y), min(depths.z, depths.w));
	const float max_depth = max(max(depths.x, depths.y), max(depths.z, depths.w));

	const float base_depth = min_depth;
	const float bias = (max_depth - min_depth);

	return base_depth > proj.z + bias ? 0.0 : 1.0;
}



#endif // SHADOWS_GLSL