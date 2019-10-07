#version 450

#include "atmosphere.glsl"

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform Data {
	LightingCamera camera;

	vec3 sun_direction;
	float sun_intensity;

	float base_height;
	float planet_height;
	float atmo_height;
} constants;

layout(location = 0) in vec2 in_uv;

void main() {
	vec3 forward = normalize(unproject(in_uv, 1.0, constants.camera.inv_matrix) - constants.camera.position);

	vec3 sky = rayleigh(constants.camera.position.z + constants.base_height,
	                    forward,
	                    constants.atmo_height,
	                    constants.planet_height,
	                    constants.sun_direction);

	out_color = vec4(sky * constants.sun_intensity, 1.0);
}
