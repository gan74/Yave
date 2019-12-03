#version 450

#include "sky.glsl"

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_color;
layout(set = 0, binding = 2) uniform sampler2D in_normal;

layout(set = 0, binding = 3) uniform sampler2D brdf_lut;

layout(set = 0, binding = 4) uniform RayleighSkyData {
	RayleighSky sky;
};

layout(set = 0, binding = 5) uniform SkyLightData {
	SkyLight light;
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;



void main() {
	const vec3 view_dir = normalize(unproject(in_uv, 1.0, sky.camera.inv_matrix) - sky.camera.position);
	const ivec2 coord = ivec2(gl_FragCoord.xy);
	const vec2 uv = in_uv;

	const float depth = texelFetch(in_depth, coord, 0).x;
	vec3 irradiance = vec3(0.0);

	if(!is_OOB(depth)) {
		vec3 albedo;
		float metallic;
		vec3 normal;
		float roughness;
		unpack_color(texelFetch(in_color, coord, 0), albedo, metallic);
		unpack_normal(texelFetch(in_normal, coord, 0), normal, roughness);

		irradiance = light.sun_color * L0(normal, light.sun_direction, view_dir, roughness, metallic, albedo);
		irradiance += sh_irradiance(light.sh, brdf_lut, normal, view_dir, roughness, metallic, albedo);
	} else {
		irradiance = rayleigh(sky.camera.position.z + sky.base_height,
		                      view_dir,
		                      sky.atmo_height,
		                      sky.planet_height,
		                      sky.sun_direction,
		                      sky.beta_rayleigh) * sky.sun_color;
	}

	out_color = vec4(irradiance, 1.0);
}
