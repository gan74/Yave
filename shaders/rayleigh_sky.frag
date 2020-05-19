#version 450

#include "sky.glsl"

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_color;
layout(set = 0, binding = 2) uniform sampler2D in_normal;

layout(set = 0, binding = 3) uniform sampler2D brdf_lut;

layout(set = 0, binding = 4) uniform CameraData {
	Camera camera;
};

layout(set = 0, binding = 5) uniform RayleighSkyData {
	RayleighSky sky;
};

layout(set = 0, binding = 6) uniform SkyLightData {
	SkyLight light;
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

// https://www.shadertoy.com/view/MstBWs
///////////////////////////////////////////////////////////////////////////////////

struct positionStruct
{
	vec3 worldVector;
	vec3 sunVector;
};

#define d0(x) (abs(x) + 1e-8)
#define d02(x) (abs(x) + 1e-3)

const float rPi = 1.0 / pi;
const float hPi = pi * 0.5;
const float tau = pi * 2.0;
const float rLOG2 = 1.0 / log(2.0);

#define cloudSpeed 0.02
#define cloudHeight 0.0
#define cloudThickness 500.0
#define cloudDensity 0.03

#define fogDensity 0.00003

#define volumetricCloudSteps 16			//Higher is a better result with rendering of clouds.
#define volumetricLightSteps 8			//Higher is a better result with rendering of volumetric light.

#define cloudShadowingSteps 12			//Higher is a better result with shading on clouds.
#define volumetricLightShadowSteps 4	//Higher is a better result with shading on volumetric light from clouds

#define rayleighCoeff (vec3(0.27, 0.5, 1.0) * 1e-5)	//Not really correct
#define mieCoeff vec3(0.5e-6)						//Not really correct

const float sunBrightness = 1.0;

#define earthRadius 6371000.0


const vec3 totalCoeff = rayleighCoeff + mieCoeff;

vec3 scatter(vec3 coeff, float depth){
	return coeff * depth;
}

vec3 absorb(vec3 coeff, float depth){
	return exp2(scatter(coeff, -depth));
}

float calcParticleThickness(float depth){

	depth = depth * 2.0;
	depth = max(depth + 0.01, 0.01);
	depth = 1.0 / depth;

	return 100000.0 * depth;
}

float calcParticleThicknessH(float depth){

	depth = depth * 2.0 + 0.1;
	depth = max(depth + 0.01, 0.01);
	depth = 1.0 / depth;

	return 100000.0 * depth;
}

float calcParticleThicknessConst(const float depth){

	return 100000.0 / max(depth * 2.0 - 0.01, 0.01);
}

float rayleighPhase(float x){
	return 0.375 * (1.0 + x*x);
}

float hgPhase(float x, float g)
{
	float g2 = g*g;
	return 0.25 * ((1.0 - g2) * pow(1.0 + g2 - 2.0*g*x, -1.5));
}

float miePhaseSky(float x, float depth)
{
	return hgPhase(x, exp2(-0.000003 * depth));
}

float powder(float od)
{
	return 1.0 - exp2(-od * 2.0);
}

float calculateScatterIntergral(float opticalDepth, float coeff){
	float a = -coeff * rLOG2;
	float b = -1.0 / coeff;
	float c =  1.0 / coeff;

	return exp2(a * opticalDepth) * b + c;
}

vec3 calculateScatterIntergral(float opticalDepth, vec3 coeff){
	vec3 a = -coeff * rLOG2;
	vec3 b = -1.0 / coeff;
	vec3 c =  1.0 / coeff;

	return exp2(a * opticalDepth) * b + c;
}


vec3 calcAtmosphericScatter(positionStruct pos, out vec3 absorbLight){
	const float ln2 = log(2.0);

	float lDotW = dot(pos.sunVector, pos.worldVector);
	float lDotU = dot(pos.sunVector, vec3(0.0, 1.0, 0.0));
	float uDotW = dot(vec3(0.0, 1.0, 0.0), pos.worldVector);

	float opticalDepth = calcParticleThickness(uDotW);
	float opticalDepthLight = calcParticleThickness(lDotU);

	vec3 scatterView = scatter(totalCoeff, opticalDepth);
	vec3 absorbView = absorb(totalCoeff, opticalDepth);

	vec3 scatterLight = scatter(totalCoeff, opticalDepthLight);
	absorbLight = absorb(totalCoeff, opticalDepthLight);

	vec3 absorbSun = abs(absorbLight - absorbView) / d0((scatterLight - scatterView) * ln2);

	vec3 mieScatter = scatter(mieCoeff, opticalDepth) * miePhaseSky(lDotW, opticalDepth);
	vec3 rayleighScatter = scatter(rayleighCoeff, opticalDepth) * rayleighPhase(lDotW);

	vec3 scatterSun = mieScatter + rayleighScatter;

	vec3 sunSpot = smoothstep(0.9999, 0.99993, lDotW) * absorbView * sunBrightness;

	return (scatterSun * absorbSun + sunSpot);
}





void main() {
	const vec3 view_dir = normalize(unproject(in_uv, 1.0, camera.inv_view_proj) - camera.position);
	const ivec2 coord = ivec2(gl_FragCoord.xy);
	const vec2 uv = in_uv;

	const float depth = texelFetch(in_depth, coord, 0).x;
	vec3 irradiance = vec3(0.0);

	if(!is_OOB(depth)) {
		/*vec3 albedo;
		float metallic;
		vec3 normal;
		float roughness;
		unpack_color(texelFetch(in_color, coord, 0), albedo, metallic);
		unpack_normal(texelFetch(in_normal, coord, 0), normal, roughness);

		irradiance = light.sun_color * L0(normal, light.sun_direction, view_dir, roughness, metallic, albedo);
		irradiance += sh_irradiance(light.sh, brdf_lut, normal, view_dir, roughness, metallic, albedo);*/
	} else {
		/*irradiance = rayleigh(camera.position.z + sky.base_height,
							  view_dir,
							  sky.atmo_height,
							  sky.planet_height,
							  sky.sun_direction,
							  sky.beta_rayleigh) * sky.sun_color;*/
		positionStruct pos;
		pos.sunVector = sky.sun_direction.xzy;
		pos.worldVector = view_dir.xzy;

		vec3 lightAbsorb = vec3(0.0);
		irradiance = calcAtmosphericScatter(pos, lightAbsorb) * sky.sun_color;
	}

	out_color = vec4(irradiance, 1.0);
}
