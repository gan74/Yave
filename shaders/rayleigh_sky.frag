#version 450

#include "atmosphere.glsl"

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform Data {
	LightingCamera camera;

	vec3 sun_direction;
	float base_height;

	vec3 sun_color;
	float planet_height;
	float atmo_height;
} constants;

layout(location = 0) in vec2 in_uv;

void main() {const vec3 forward = normalize(unproject(in_uv, 1.0, constants.camera.inv_matrix) - constants.camera.position);

	const vec3 sky = rayleigh(constants.camera.position.z + constants.base_height,
							  forward,
							  constants.atmo_height,
							  constants.planet_height,
							  constants.sun_direction);

	out_color = vec4(sky * constants.sun_color, 1.0);
}
