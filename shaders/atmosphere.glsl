#include "yave.glsl"

bool ray_sphere(vec3 start, vec3 dir, float radius, out vec2 intersections) {
	float a = dot(dir, dir);
	float b = 2.0 * dot(dir, start);
	float c = dot(start, start) - sqr(radius);
	float d = sqr(b) - 4.0 * a * c;

	if(d < 0.0) {
		intersections = vec2(1e8, -1e8);
		return false;
	}

	intersections =  vec2(
	        (-b - sqrt(d)) / (2.0 * a),
	        (-b + sqrt(d)) / (2.0 * a)
		);
	return true;
}

vec2 ray_sphere(vec3 start, vec3 dir, float radius) {
	float a = dot(dir, dir);
	float b = 2.0 * dot(dir, start);
	float c = dot(start, start) - sqr(radius);
	float d = sqr(b) - 4.0 * a * c;

	if(d < 0.0) {
		return vec2(1e8, -1e8);
	}

	return vec2(
	        (-b - sqrt(d)) / (2.0 * a),
	        (-b + sqrt(d)) / (2.0 * a)
	    );
}

// https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky
// https://github.com/wwwtyro/glsl-atmosphere
vec3 rayleigh(float origin_height, vec3 dir, float atmos_radius, float planet_radius, vec3 sun_dir, float t_min, float t_max) {
	/*{
		vec2 intersections;
		if(ray_sphere(vec3(0.0, 0.0, origin_height), dir, planet_radius, intersections) && intersections.y > 0.0) {
			t_max = max(0.0, intersections.x);
		}
	}

	{
		vec2 intersections;
		if(!ray_sphere(vec3(0.0, 0.0, origin_height), dir, atmos_radius, intersections) || intersections.y < 0.0) {
			return vec3(0.0);
		}

		t_min = max(t_min, intersections.x);
		t_max = min(t_max, intersections.y);
	}*/

	{
		vec2 intersections = ray_sphere(vec3(0.0, 0.0, origin_height), dir, planet_radius);
		if(intersections.y > 0.0) {
			t_max = max(0.0, intersections.x);
		}
	}

	{
		vec2 intersections = ray_sphere(vec3(0.0, 0.0, origin_height), dir, atmos_radius);
		if(intersections.y < 0.0) {
			return vec3(0.0);
		}

		t_min = max(t_min, intersections.x);
		t_max = min(t_max, intersections.y);
	}

	const uint step_count = 6;
	const uint light_step_count = 2;

	const float mie_g = 0.76;
	const float H_ray = 7994.0;
	const float H_mie = 1200.0;
	//const vec3 beta_ray = vec3(5.5e-6, 13.0e-6, 22.4e-6);
	const vec3 beta_ray = vec3(3.8e-6, 13.5e-6, 33.1e-6);
	const vec3 beta_mie = vec3(21e-6);

	vec3 ray = vec3(0.0);
	vec3 mie = vec3(0.0);

	float optical_ray = 0.0;
	float optical_mie = 0.0;

	float step_size = (t_max - t_min) / step_count;

	float l = t_min;
	for(uint i = 0; i != step_count; ++i) {
		vec3 pos = vec3(0.0, 0.0, origin_height) + dir * (l + step_size * 0.5);

		float height = length(pos) - planet_radius;

		float optical_step_ray = exp(-height / H_ray) * step_size;
		float optical_step_mie = exp(-height / H_mie) * step_size;

		optical_ray += optical_step_ray;
		optical_mie += optical_step_mie;

		{
			float light_step_size = ray_sphere(pos, sun_dir, atmos_radius).y / float(light_step_count);
			float light_l = 0.0;
			float light_optical_ray = 0.0;
			float light_optical_mie = 0.0;

			for(uint j = 0; j != light_step_count; ++j) {
				vec3 light_pos = pos + sun_dir * (light_l * light_step_size * 0.5);

				float light_height = length(light_pos) - planet_radius;

				light_optical_ray += exp(-light_height / H_ray) * light_step_size;
				light_optical_mie += exp(-light_height / H_mie) * light_step_size;

				light_l += light_step_size;
			}

			vec3 tau = beta_ray * (optical_ray + light_optical_ray) + beta_mie * 1.1f * (optical_mie + light_optical_mie);
			vec3 atten = exp(-tau);

			ray += atten * optical_step_ray;
			mie += atten * optical_step_mie;
		}

		l += step_size;
	}

	float mu = dot(dir, sun_dir);
	float mu2 = sqr(mu);
	float mie_g2 = sqr(mie_g);

	float phase_ray = 3.0 / (16.0 * pi) * (1.0 + mu2);
	float phase_mie = 3.0 / (8.0 * pi) * ((1.0 - mie_g2) * (1.0 + mu2)) / (pow(1.0 + mie_g2 - 2.0 * mu * mie_g, 1.5) * (2.0 + mie_g2));

	return (ray * beta_ray * phase_ray) +
	       (mie * beta_mie * phase_mie);
}

