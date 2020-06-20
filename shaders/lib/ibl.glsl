#ifndef IBL_GLSL
#define IBL_GLSL
 
#include "utils.glsl"
#include "lighting.glsl"
#include "sh.glsl"


vec3 importance_sample_GGX(vec2 Xi, vec3 normal, float roughness) {
	const float a2 = sqr(sqr(roughness));

	const float phi = 2.0 * pi * Xi.x;
	const float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a2 - 1.0) * Xi.y));
	const float sin_theta = sqrt(1.0 - sqr(cos_theta));

	const vec3 half_vec = vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

	const vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	const vec3 tangent = normalize(cross(up, normal));
	const vec3 bitangent = cross(normal, tangent);

	return normalize(tangent * half_vec.x + bitangent * half_vec.y + normal * half_vec.z);
}

vec2 integrate_brdf(float NoV, float roughness) {
	const vec3 V = vec3(sqrt(1.0 - NoV * NoV), 0.0, NoV);

	vec2 integr = vec2(0.0);

	const vec3 N = vec3(0.0, 0.0, 1.0);
	const uint SAMPLE_COUNT = 1024;
	for(uint i = 0; i != SAMPLE_COUNT; ++i) {
		const vec2 Xi = hammersley(i, SAMPLE_COUNT);
		const vec3 H  = importance_sample_GGX(Xi, N, roughness);
		const vec3 L  = normalize(2.0 * dot(V, H) * H - V);

		const float NoL = max(0.0, L.z);
		const float NoH = max(0.0, H.z);
		const float VoH = max(0.0, dot(V, H));

		if(NoL > 0.0) {
			const float k_ibl = sqr(roughness) * 0.5;
			const float G = G_GGX(NoV, NoL, k_ibl);
			const float G_Vis = (G * VoH) / (NoH * NoV);
			const float Fc = pow(1.0 - VoH, 5.0);

			integr += vec2((1.0 - Fc) * G_Vis, Fc * G_Vis);
		}
	}
	return integr / SAMPLE_COUNT;
}

vec3 ibl_irradiance(samplerCube probe, sampler2D brdf_lut, vec3 normal, vec3 view_dir, float roughness, float metallic, vec3 albedo) {
	const uint probe_mips = textureQueryLevels(probe);
	const float NoV = max(0.0, dot(normal, view_dir));

	const vec3 kS = F_Schlick(NoV, approx_F0(metallic, albedo), roughness);
	const vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

	const vec3 irradiance = textureLod(probe, normal, probe_mips - 1).rgb;
	const vec3 diffuse = kD * irradiance * albedo;

	const vec3 reflected = reflect(-view_dir, normal);
	const vec2 brdf = texture(brdf_lut, vec2(NoV, roughness)).xy; // TODO: make it so we don't wrap (which breaks roughness close to 0 or 1)
	const vec3 specular = (kS * brdf.x + brdf.y) * textureLod(probe, reflected, roughness * probe_mips).rgb;

	return diffuse + specular;
}

vec3 sh_irradiance(SH probe, sampler2D brdf_lut, vec3 normal, vec3 view_dir, float roughness, float metallic, vec3 albedo) {
	const float NoV = max(0.0, dot(normal, view_dir));

	const vec3 kS = F_Schlick(NoV, approx_F0(metallic, albedo), roughness);
	const vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

	const vec3 irradiance = eval_sh(probe, normal);
	const vec3 diffuse = kD * irradiance * albedo;

	const vec3 reflected = reflect(-view_dir, normal);
	const vec2 brdf = texture(brdf_lut, vec2(NoV, roughness)).xy; // TODO: make it so we don't wrap (which breaks roughness close to 0 or 1)
	const vec3 specular = (kS * brdf.x + brdf.y) * eval_sh(probe, reflected);

	return diffuse + specular;
}


vec3 diffuse_convolution(samplerCube envmap, vec3 normal) {
	vec3 acc = vec3(0.0);
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	const vec3 right = normalize(cross(up, normal));
	up = cross(normal, right);
	const float sample_delta = 0.05;
	float samples = 0.0;
	for(float phi = 0.0; phi < 2.0 * pi; phi += sample_delta) {
		for(float theta = 0.0; theta < 0.5 * pi; theta += sample_delta) {
			vec3 tangent_sample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			vec3 sample_dir = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * normal;

			acc += texture(envmap, sample_dir).rgb * cos(theta) * sin(theta);
			++samples;
		}
	}
	return acc / samples * pi;
}

vec3 diffuse_convolution(sampler2D envmap, vec3 normal) {
	vec3 acc = vec3(0.0);
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	const vec3 right = normalize(cross(up, normal));
	up = cross(normal, right);
	const float sample_delta = 0.05;
	float samples = 0.0;
	for(float phi = 0.0; phi < 2.0 * pi; phi += sample_delta) {
		for(float theta = 0.0; theta < 0.5 * pi; theta += sample_delta) {
			vec3 tangent_sample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			vec3 sample_dir = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * normal;

			acc += texture(envmap, to_equirec(sample_dir)).rgb * cos(theta) * sin(theta);
			++samples;
		}
	}
	return acc / samples * pi;
}





vec3 specular_convolution(samplerCube envmap, vec3 N, float roughness) {
	const vec3 V = N;
	float total = 0.0;
	vec3 acc = vec3(0.0);
	const uint sample_count = 1024;
	for(uint i = 0; i != sample_count; ++i) {
		vec2 Xi = hammersley(i, sample_count);
		vec3 H  = importance_sample_GGX(Xi, N, roughness);
		vec3 L  = normalize(2.0 * dot(V, H) * H - V);
		float NoL = dot(N, L);
		if(NoL > 0.0) {
			acc += texture(envmap, L).rgb * NoL;
			total += NoL;
		}
	}
	return acc / total;
}

vec3 specular_convolution(sampler2D envmap, vec3 N, float roughness) {
	const vec3 V = N;
	float total = 0.0;
	vec3 acc = vec3(0.0);
	const uint sample_count = 1024;
	for(uint i = 0; i != sample_count; ++i) {
		vec2 Xi = hammersley(i, sample_count);
		vec3 H  = importance_sample_GGX(Xi, N, roughness);
		vec3 L  = normalize(2.0 * dot(V, H) * H - V);
		float NoL = dot(N, L);
		if(NoL > 0.0) {
			acc += texture(envmap, to_equirec(L)).rgb * NoL;
			total += NoL;
		}
	}
	return acc / total;
}

#endif // IBL_GLSL