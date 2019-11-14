#ifndef ATMOSPHERE_GLSL
#define ATMOSPHERE_GLSL

#include "yave.glsl"

vec2 ray_sphere(vec3 origin, vec3 dir, float radius) {
	const float a = dot(dir, dir);
	const float b = 2.0 * dot(dir, origin);
	const float c = dot(origin, origin) - sqr(radius);
	const float d = sqr(b) - 4.0 * a * c;

	if (d < 0.0) {
		return vec2(1e5, -1e5);
	}

	return vec2(
	        (-b - sqrt(d)) / (2.0 * a),
	        (-b + sqrt(d)) / (2.0 * a)
	    );
}

// https://github.com/wwwtyro/glsl-atmosphere
// https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky
vec3 rayleigh(float origin_height, vec3 dir, float atmo_radius, float planet_radius, vec3 sun_dir) {

	const uint step_count = 16;
	const uint light_step_count = 2;

	const vec3 origin = vec3(0.0, 0.0, origin_height);
	const float g_mie = 0.758;
	const float H_ray = 8e3;
	const float H_mie = 1.2e3;

	const vec3 beta_ray = vec3(5.5e-6, 13.0e-6, 22.4e-6);
	const float beta_mie = 21e-6;

	vec2 p = ray_sphere(origin, dir, atmo_radius);
	if (p.x > p.y) {
		return vec3(0.0);
	}
	p.y = min(p.y, ray_sphere(origin, dir, planet_radius).x);

	/*{
		vec2 planet = ray_sphere(origin, dir, planet_radius);
		if(planet.y > 0.0) {
			p.y = min(planet.x, p.y);
		}
	}*/

	vec3 ray = vec3(0.0);
	vec3 mie = vec3(0.0);

	float current = 0.0;
	float optical_ray = 0.0;
	float optical_mie = 0.0;
	const float step_size = (p.y - p.x) / float(step_count);

	for (uint i = 0; i != step_count; ++i) {
		const vec3 pos = origin + dir * (current + step_size * 0.5);

		const float height = length(pos) - planet_radius;

		const float optical_step_ray = exp(-height / H_ray) * step_size;
		const float optical_step_mie = exp(-height / H_mie) * step_size;

		optical_ray += optical_step_ray;
		optical_mie += optical_step_mie;

		{
			float light_current = 0.0;
			float light_optical_ray = 0.0;
			float light_optical_mie = 0.0;
			const float light_step_size = ray_sphere(pos, sun_dir, atmo_radius).y / float(light_step_count);

			for (uint j = 0; j != light_step_count; ++j) {
				const vec3 light_pos = pos + sun_dir * (light_current + light_step_size * 0.5);

				const float light_height = length(light_pos) - planet_radius;

				light_optical_ray += exp(-light_height / H_ray) * light_step_size;
				light_optical_mie += exp(-light_height / H_mie) * light_step_size;

				light_current += light_step_size;
			}

			const vec3 tau = beta_ray * (optical_ray + light_optical_ray) +
			           beta_mie * (optical_mie + light_optical_mie);
			const vec3 atten = exp(-tau);

			ray += optical_step_ray * atten;
			mie += optical_step_mie * atten;
		}

		current += step_size;
	}

	const float mu = dot(dir, sun_dir);
	const float mu2 = sqr(mu);
	const float g_mie2 = sqr(g_mie);
	const float phase_ray = 3.0 / (16.0 * pi) * (1.0 + mu2);
	const float phase_mie = 3.0 / (8.0 * pi) * ((1.0 - g_mie2) * (mu2 + 1.0)) / (pow(1.0 + g_mie2 - 2.0 * mu * g_mie, 1.5) * (2.0 + g_mie2));

	return (ray * beta_ray * phase_ray) +
	       (mie * beta_mie * phase_mie);
}

#endif

