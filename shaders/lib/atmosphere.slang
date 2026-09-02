/*
 * Copyright (c) 2017 Eric Bruneton / INRIA. BSD-style license.
 * Precomputed Atmospheric Scattering — https://github.com/ebruneton/precomputed_atmospheric_scattering
 * Converted to Slang for Yave. Combined scattering (Mie in alpha), radiance-mode RGB.
 */

#ifndef ATMOSPHERE_LIB_SLANG
#define ATMOSPHERE_LIB_SLANG

#include "utils.slang"

float rayleigh_phase(float nu);

namespace atmosphere {

// 3D scattering LUT packs nu * mu_s along width; only nu count is fixed.
static const int scattering_nu_size = 8;

static const float3 rayleigh_scattering = float3(5.8023394e-6, 1.3557762e-5, 3.3100006e-5);
static const float3 mie_scattering = float3(3.996e-6, 3.996e-6, 3.996e-6);
static const float3 mie_extinction = float3(4.44e-6, 4.44e-6, 4.44e-6);
static const float mie_phase_g = 0.8;
static const float3 absorption_extinction = float3(6.497166e-7, 1.8809e-6, 8.501668e-8);
static const float3 ground_albedo = float3(0.1, 0.1, 0.1);
static const float mu_s_min = -0.20791169;

struct DensityLayer {
    float width;
    float exp_term;
    float exp_scale;
    float linear_term;
    float constant_term;
};

struct DensityProfile {
    DensityLayer layers[2];
};

static const DensityProfile rayleigh_density = {
    {{0.0, 1.0, -1.0 / 8000.0, 0.0, 0.0},
     {0.0, 1.0, -1.0 / 8000.0, 0.0, 0.0}}
};

static const DensityProfile mie_density = {
    {{0.0, 1.0, -1.0 / 1200.0, 0.0, 0.0},
     {0.0, 1.0, -1.0 / 1200.0, 0.0, 0.0}}
};

static const DensityProfile absorption_density = {
    {{25000.0, 0.0, 0.0, 1.0 / 15000.0, -2.0 / 3.0},
     {0.0, 0.0, 0.0, -1.0 / 15000.0, 8.0 / 3.0}}
};

float clamp_radius(AtmosphereParams atmo, float r) {
    return clamp(r, atmo.bottom_radius, atmo.top_radius);
}

float distance_to_top(AtmosphereParams atmo, float r, float mu) {
    const float discriminant = r * r * (mu * mu - 1.0) + atmo.top_radius * atmo.top_radius;
    return max(-r * mu + sqrt(max(discriminant, 0.0)), 0.0);
}

float distance_to_bottom(AtmosphereParams atmo, float r, float mu) {
    const float discriminant = r * r * (mu * mu - 1.0) + atmo.bottom_radius * atmo.bottom_radius;
    return max(-r * mu - sqrt(max(discriminant, 0.0)), 0.0);
}

bool intersects_ground(AtmosphereParams atmo, float r, float mu) {
    return mu < 0.0
        && r * r * (mu * mu - 1.0) + atmo.bottom_radius * atmo.bottom_radius >= 0.0;
}

float distance_to_boundary(AtmosphereParams atmo, float r, float mu, bool ray_intersects_ground) {
    return ray_intersects_ground ? distance_to_bottom(atmo, r, mu) : distance_to_top(atmo, r, mu);
}

float layer_density(DensityLayer layer, float altitude) {
    const float density = layer.exp_term * exp(layer.exp_scale * altitude)
        + layer.linear_term * altitude + layer.constant_term;
    return clamp(density, 0.0, 1.0);
}

float profile_density(DensityProfile profile, float altitude) {
    return altitude < profile.layers[0].width
        ? layer_density(profile.layers[0], altitude)
        : layer_density(profile.layers[1], altitude);
}

float optical_length_to_top(AtmosphereParams atmo, DensityProfile profile, float r, float mu) {
    const int sample_count = 500;
    const float dx = distance_to_top(atmo, r, mu) / float(sample_count);
    float result = 0.0;
    for(int i = 0; i <= sample_count; ++i) {
        const float d_i = float(i) * dx;
        const float r_i = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r);
        const float y_i = profile_density(profile, r_i - atmo.bottom_radius);
        const float weight_i = (i == 0 || i == sample_count) ? 0.5 : 1.0;
        result += y_i * weight_i * dx;
    }
    return result;
}

float3 transmittance_to_top(AtmosphereParams atmo, float r, float mu) {
    float3 optical_depth = 0.0;
    optical_depth += rayleigh_scattering * optical_length_to_top(atmo, rayleigh_density, r, mu);
    optical_depth += mie_extinction * optical_length_to_top(atmo, mie_density, r, mu);
    optical_depth += absorption_extinction * optical_length_to_top(atmo, absorption_density, r, mu);
    return exp(-optical_depth);
}

float texture_coord_from_unit_range(float x, int texture_size) {
    return 0.5 / float(texture_size) + x * (1.0 - 1.0 / float(texture_size));
}

float unit_range_from_texture_coord(float u, int texture_size) {
    return (u - 0.5 / float(texture_size)) / (1.0 - 1.0 / float(texture_size));
}

float2 transmittance_uv_from_r_mu(AtmosphereParams atmo, float r, float mu, uint2 size) {
    const float H = sqrt(atmo.top_radius * atmo.top_radius
        - atmo.bottom_radius * atmo.bottom_radius);
    const float rho = sqrt(max(r * r - atmo.bottom_radius * atmo.bottom_radius, 0.0));
    const float d = distance_to_top(atmo, r, mu);
    const float d_min = atmo.top_radius - r;
    const float d_max = rho + H;
    const float x_mu = (d - d_min) / (d_max - d_min);
    const float x_r = rho / H;
    return float2(
        texture_coord_from_unit_range(x_mu, int(size.x)),
        texture_coord_from_unit_range(x_r, int(size.y)));
}

void r_mu_from_transmittance_uv(AtmosphereParams atmo, float2 uv, uint2 size, out float r, out float mu) {
    const float x_mu = unit_range_from_texture_coord(uv.x, int(size.x));
    const float x_r = unit_range_from_texture_coord(uv.y, int(size.y));
    const float H = sqrt(atmo.top_radius * atmo.top_radius
        - atmo.bottom_radius * atmo.bottom_radius);
    const float rho = H * x_r;
    r = sqrt(rho * rho + atmo.bottom_radius * atmo.bottom_radius);
    const float d_min = atmo.top_radius - r;
    const float d_max = rho + H;
    const float d = d_min + x_mu * (d_max - d_min);
    mu = d == 0.0 ? 1.0 : (H * H - rho * rho - d * d) / (2.0 * r * d);
    mu = clamp(mu, -1.0, 1.0);
}

float3 sample_transmittance_to_top(AtmosphereParams atmo, Sampler2D transmittance, float r, float mu) {
    return transmittance.SampleLevel(transmittance_uv_from_r_mu(atmo, r, mu, image_size(transmittance)), 0).rgb;
}

float3 sample_transmittance(AtmosphereParams atmo, Sampler2D transmittance, float r, float mu, float d, bool ray_intersects_ground) {
    const float r_d = clamp_radius(atmo, sqrt(d * d + 2.0 * r * mu * d + r * r));
    const float mu_d = clamp((r * mu + d) / r_d, -1.0, 1.0);
    if(ray_intersects_ground) {
        return min(
            sample_transmittance_to_top(atmo, transmittance, r_d, -mu_d) /
            sample_transmittance_to_top(atmo, transmittance, r, -mu),
            float3(1, 1, 1));
    }
    return min(
        sample_transmittance_to_top(atmo, transmittance, r, mu) /
        sample_transmittance_to_top(atmo, transmittance, r_d, mu_d),
        float3(1, 1, 1));
}

float3 sample_transmittance_to_sun(AtmosphereParams atmo, Sampler2D transmittance, float r, float mu_s) {
    const float sin_theta_h = atmo.bottom_radius / r;
    const float cos_theta_h = -sqrt(max(1.0 - sin_theta_h * sin_theta_h, 0.0));
    const float step = smoothstep(
        -sin_theta_h * atmo.sun_angular_radius,
        sin_theta_h * atmo.sun_angular_radius,
        mu_s - cos_theta_h);
    return sample_transmittance_to_top(atmo, transmittance, r, mu_s) * step;
}

float mie_phase(float g, float nu) {
    const float k = 3.0 / (8.0 * constants.pi) * (1.0 - g * g) / (2.0 + g * g);
    return k * (1.0 + nu * nu) / pow(abs(1.0 + g * g - 2.0 * g * nu), 1.5);
}

void single_scattering_integrand(AtmosphereParams atmo, 
    Sampler2D transmittance,
    float r, float mu, float mu_s, float nu, float d,
    bool ray_intersects_ground,
    out float3 rayleigh, out float3 mie)
{
    const float r_d = clamp_radius(atmo, sqrt(d * d + 2.0 * r * mu * d + r * r));
    const float mu_s_d = clamp((r * mu_s + d * nu) / r_d, -1.0, 1.0);
    const float3 t =
        sample_transmittance(atmo, transmittance, r, mu, d, ray_intersects_ground) *
        sample_transmittance_to_sun(atmo, transmittance, r_d, mu_s_d);
    rayleigh = t * profile_density(rayleigh_density, r_d - atmo.bottom_radius);
    mie = t * profile_density(mie_density, r_d - atmo.bottom_radius);
}

void single_scattering(AtmosphereParams atmo, 
    Sampler2D transmittance,
    float r, float mu, float mu_s, float nu,
    bool ray_intersects_ground,
    out float3 rayleigh, out float3 mie)
{
    const int sample_count = 50;
    const float dx = distance_to_boundary(atmo, r, mu, ray_intersects_ground) / float(sample_count);
    float3 rayleigh_sum = float3(0, 0, 0);
    float3 mie_sum = float3(0, 0, 0);
    for(int i = 0; i <= sample_count; ++i) {
        const float d_i = float(i) * dx;
        float3 rayleigh_i;
        float3 mie_i;
        single_scattering_integrand(atmo, 
            transmittance, r, mu, mu_s, nu, d_i,
            ray_intersects_ground, rayleigh_i, mie_i);
        const float weight_i = (i == 0 || i == sample_count) ? 0.5 : 1.0;
        rayleigh_sum += rayleigh_i * weight_i;
        mie_sum += mie_i * weight_i;
    }
    rayleigh = rayleigh_sum * dx * atmo.solar_irradiance * rayleigh_scattering;
    mie = mie_sum * dx * atmo.solar_irradiance * mie_scattering;
}

float4 scattering_uvwz_from_r_mu_mu_s_nu(AtmosphereParams atmo, 
    float r, float mu, float mu_s, float nu,
    bool ray_intersects_ground,
    uint3 size)
{
    const int mu_s_size = int(size.x) / scattering_nu_size;
    const float H = sqrt(atmo.top_radius * atmo.top_radius
        - atmo.bottom_radius * atmo.bottom_radius);
    const float rho = sqrt(max(r * r - atmo.bottom_radius * atmo.bottom_radius, 0.0));
    const float u_r = texture_coord_from_unit_range(rho / H, int(size.z));

    // Non-ground view rays only; map mu across full [0,1] height.
    const float r_mu = r * mu;
    const float discriminant = r_mu * r_mu - r * r + atmo.bottom_radius * atmo.bottom_radius;
    const float d = -r_mu + sqrt(max(discriminant + H * H, 0.0));
    const float d_min = atmo.top_radius - r;
    const float d_max = rho + H;
    const float u_mu = texture_coord_from_unit_range((d - d_min) / (d_max - d_min), int(size.y));

    const float d_sun = distance_to_top(atmo, atmo.bottom_radius, mu_s);
    const float d_min_s = atmo.top_radius - atmo.bottom_radius;
    const float d_max_s = H;
    const float a = (d_sun - d_min_s) / (d_max_s - d_min_s);
    const float A = -2.0 * mu_s_min * atmo.bottom_radius / (d_max_s - d_min_s);
    const float u_mu_s = texture_coord_from_unit_range(max(1.0 - a / A, 0.0) / (1.0 + a), mu_s_size);

    const float u_nu = (nu + 1.0) / 2.0;
    return float4(u_nu, u_mu_s, u_mu, u_r);
}

void r_mu_mu_s_nu_from_scattering_uvwz(AtmosphereParams atmo, 
    float4 uvwz,
    uint3 size,
    out float r, out float mu, out float mu_s, out float nu,
    out bool ray_intersects_ground)
{
    const int mu_s_size = int(size.x) / scattering_nu_size;
    const float H = sqrt(atmo.top_radius * atmo.top_radius
        - atmo.bottom_radius * atmo.bottom_radius);
    const float rho = H * unit_range_from_texture_coord(uvwz.w, int(size.z));
    r = sqrt(rho * rho + atmo.bottom_radius * atmo.bottom_radius);

    const float d_min = atmo.top_radius - r;
    const float d_max = rho + H;
    const float d = d_min + (d_max - d_min) * unit_range_from_texture_coord(uvwz.z, int(size.y));
    mu = d == 0.0 ? 1.0 : clamp((H * H - rho * rho - d * d) / (2.0 * r * d), -1.0, 1.0);
    ray_intersects_ground = false;

    const float x_mu_s = unit_range_from_texture_coord(uvwz.y, mu_s_size);
    const float d_min_s = atmo.top_radius - atmo.bottom_radius;
    const float d_max_s = H;
    const float A = -2.0 * mu_s_min * atmo.bottom_radius / (d_max_s - d_min_s);
    const float a = (A - x_mu_s * A) / (1.0 + x_mu_s * A);
    const float d_s = d_min_s + min(a, A) * (d_max_s - d_min_s);
    mu_s = d_s == 0.0 ? 1.0 : clamp((H * H - d_s * d_s) / (2.0 * atmo.bottom_radius * d_s), -1.0, 1.0);

    nu = clamp(uvwz.x * 2.0 - 1.0, -1.0, 1.0);
}

void r_mu_mu_s_nu_from_scattering_frag_coord(AtmosphereParams atmo, 
    float3 frag_coord,
    uint3 size,
    out float r, out float mu, out float mu_s, out float nu,
    out bool ray_intersects_ground)
{
    const int mu_s_size = int(size.x) / scattering_nu_size;
    const float4 scale = float4(scattering_nu_size - 1, mu_s_size, int(size.y), int(size.z));

    const float frag_coord_nu = floor(frag_coord.x / float(mu_s_size));
    const float frag_coord_mu_s = fmod(frag_coord.x, float(mu_s_size));
    const float4 uvwz = float4(frag_coord_nu, frag_coord_mu_s, frag_coord.y, frag_coord.z) / scale;

    r_mu_mu_s_nu_from_scattering_uvwz(atmo, uvwz, size, r, mu, mu_s, nu, ray_intersects_ground);
    nu = clamp(
        nu,
        mu * mu_s - sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)),
        mu * mu_s + sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)));
}

float3 sample_scattering(AtmosphereParams atmo, 
    Sampler3D scattering,
    float r, float mu, float mu_s, float nu,
    bool ray_intersects_ground)
{
    if(ray_intersects_ground) {
        return float3(0, 0, 0);
    }

    const uint3 size = image_size(scattering);
    const float4 uvwz = scattering_uvwz_from_r_mu_mu_s_nu(atmo, r, mu, mu_s, nu, ray_intersects_ground, size);
    const float tex_coord_x = uvwz.x * float(scattering_nu_size - 1);
    const float tex_x = floor(tex_coord_x);
    const float blend_t = tex_coord_x - tex_x;
    const float3 uvw0 = float3((tex_x + uvwz.y) / float(scattering_nu_size), uvwz.z, uvwz.w);
    const float3 uvw1 = float3((tex_x + 1.0 + uvwz.y) / float(scattering_nu_size), uvwz.z, uvwz.w);

    return scattering.SampleLevel(uvw0, 0).rgb * (1.0 - blend_t)
         + scattering.SampleLevel(uvw1, 0).rgb * blend_t;
}

float3 sample_scattering(AtmosphereParams atmo, 
    Sampler3D delta_rayleigh,
    Sampler3D delta_mie,
    Sampler3D delta_multiple,
    float r, float mu, float mu_s, float nu,
    bool ray_intersects_ground,
    int scattering_order)
{
    if(scattering_order == 1) {
        const float3 rayleigh = sample_scattering(atmo, delta_rayleigh, r, mu, mu_s, nu, ray_intersects_ground);
        const float3 mie = sample_scattering(atmo, delta_mie, r, mu, mu_s, nu, ray_intersects_ground);
        return rayleigh * rayleigh_phase(nu) + mie * mie_phase(mie_phase_g, nu);
    }
    return sample_scattering(atmo, delta_multiple, r, mu, mu_s, nu, ray_intersects_ground);
}

float3 direct_irradiance(AtmosphereParams atmo, Sampler2D transmittance, float r, float mu_s) {
    const float alpha_s = atmo.sun_angular_radius;
    const float average_cosine_factor =
        mu_s < -alpha_s ? 0.0
        : (mu_s > alpha_s ? mu_s : (mu_s + alpha_s) * (mu_s + alpha_s) / (4.0 * alpha_s));
    return atmo.solar_irradiance
        * sample_transmittance_to_top(atmo, transmittance, r, mu_s)
        * average_cosine_factor;
}

float3 indirect_irradiance(AtmosphereParams atmo, 
    Sampler3D delta_rayleigh,
    Sampler3D delta_mie,
    Sampler3D delta_multiple,
    float r, float mu_s, int scattering_order)
{
    const int sample_count = 32;
    const float dphi = constants.pi / float(sample_count);
    const float dtheta = constants.pi / float(sample_count);
    float3 result = float3(0, 0, 0);
    const float3 omega_s = float3(sqrt(1.0 - mu_s * mu_s), 0.0, mu_s);

    for(int j = 0; j < sample_count / 2; ++j) {
        const float theta = (float(j) + 0.5) * dtheta;
        for(int i = 0; i < 2 * sample_count; ++i) {
            const float phi = (float(i) + 0.5) * dphi;
            const float3 omega = float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
            const float domega = dtheta * dphi * sin(theta);
            const float nu = dot(omega, omega_s);
            result += sample_scattering(atmo, 
                delta_rayleigh, delta_mie, delta_multiple,
                r, omega.z, mu_s, nu, false, scattering_order)
                * omega.z * domega;
        }
    }
    return result;
}

float2 irradiance_uv_from_r_mu_s(AtmosphereParams atmo, float r, float mu_s, uint2 size) {
    const float x_r = (r - atmo.bottom_radius) / (atmo.top_radius - atmo.bottom_radius);
    const float x_mu_s = mu_s * 0.5 + 0.5;
    return float2(
        texture_coord_from_unit_range(x_mu_s, int(size.x)),
        texture_coord_from_unit_range(x_r, int(size.y)));
}

void r_mu_s_from_irradiance_uv(AtmosphereParams atmo, float2 uv, uint2 size, out float r, out float mu_s) {
    const float x_mu_s = unit_range_from_texture_coord(uv.x, int(size.x));
    const float x_r = unit_range_from_texture_coord(uv.y, int(size.y));
    r = atmo.bottom_radius + x_r * (atmo.top_radius - atmo.bottom_radius);
    mu_s = clamp(2.0 * x_mu_s - 1.0, -1.0, 1.0);
}

float3 sample_irradiance(AtmosphereParams atmo, Sampler2D irradiance, float r, float mu_s) {
    return irradiance.SampleLevel(irradiance_uv_from_r_mu_s(atmo, r, mu_s, image_size(irradiance)), 0).rgb;
}

float3 scattering_density(AtmosphereParams atmo, 
    Sampler2D transmittance,
    Sampler3D delta_rayleigh,
    Sampler3D delta_mie,
    Sampler3D delta_multiple,
    Sampler2D irradiance,
    float r, float mu, float mu_s, float nu, int scattering_order)
{
    const float3 zenith = float3(0.0, 0.0, 1.0);
    const float3 omega = float3(sqrt(1.0 - mu * mu), 0.0, mu);
    const float sun_dir_x = omega.x == 0.0 ? 0.0 : (nu - mu * mu_s) / omega.x;
    const float sun_dir_y = sqrt(max(1.0 - sun_dir_x * sun_dir_x - mu_s * mu_s, 0.0));
    const float3 omega_s = float3(sun_dir_x, sun_dir_y, mu_s);

    const int sample_count = 16;
    const float dphi = constants.pi / float(sample_count);
    const float dtheta = constants.pi / float(sample_count);
    float3 rayleigh_mie = float3(0, 0, 0);

    for(int l = 0; l < sample_count; ++l) {
        const float theta = (float(l) + 0.5) * dtheta;
        const float cos_theta = cos(theta);
        const float sin_theta = sin(theta);
        const bool ray_intersects_ground = intersects_ground(atmo, r, cos_theta);

        float distance_to_ground = 0.0;
        float3 transmittance_to_ground = float3(0, 0, 0);
        float3 surface_albedo = float3(0, 0, 0);
        if(ray_intersects_ground) {
            distance_to_ground = distance_to_bottom(atmo, r, cos_theta);
            transmittance_to_ground = sample_transmittance(atmo, transmittance, r, cos_theta, distance_to_ground, true);
            surface_albedo = ground_albedo;
        }

        for(int m = 0; m < 2 * sample_count; ++m) {
            const float phi = (float(m) + 0.5) * dphi;
            const float3 omega_i = float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
            const float domega_i = dtheta * dphi * sin(theta);

            const float nu1 = dot(omega_s, omega_i);
            float3 incident_radiance = sample_scattering(atmo, 
                delta_rayleigh, delta_mie, delta_multiple,
                r, omega_i.z, mu_s, nu1, ray_intersects_ground, scattering_order - 1);

            const float3 ground_normal = normalize(zenith * r + omega_i * distance_to_ground);
            const float3 ground_irradiance = sample_irradiance(atmo, 
                irradiance, atmo.bottom_radius, dot(ground_normal, omega_s));
            incident_radiance += transmittance_to_ground * surface_albedo * (1.0 / constants.pi) * ground_irradiance;

            const float nu2 = dot(omega, omega_i);
            const float rayleigh_d = profile_density(rayleigh_density, r - atmo.bottom_radius);
            const float mie_d = profile_density(mie_density, r - atmo.bottom_radius);
            rayleigh_mie += incident_radiance * (
                rayleigh_scattering * rayleigh_d * rayleigh_phase(nu2) +
                mie_scattering * mie_d * mie_phase(mie_phase_g, nu2)) * domega_i;
        }
    }
    return rayleigh_mie;
}

float3 multiple_scattering(AtmosphereParams atmo, 
    Sampler2D transmittance,
    Sampler3D density,
    float r, float mu, float mu_s, float nu,
    bool ray_intersects_ground)
{
    const int sample_count = 50;
    const float dx = distance_to_boundary(atmo, r, mu, ray_intersects_ground) / float(sample_count);
    float3 rayleigh_mie_sum = float3(0, 0, 0);

    for(int i = 0; i <= sample_count; ++i) {
        const float d_i = float(i) * dx;
        const float r_i = clamp_radius(atmo, sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r));
        const float mu_i = clamp((r * mu + d_i) / r_i, -1.0, 1.0);
        const float mu_s_i = clamp((r * mu_s + d_i * nu) / r_i, -1.0, 1.0);
        const float3 rayleigh_mie_i =
            sample_scattering(atmo, density, r_i, mu_i, mu_s_i, nu, ray_intersects_ground) *
            sample_transmittance(atmo, transmittance, r, mu, d_i, ray_intersects_ground) * dx;
        const float weight_i = (i == 0 || i == sample_count) ? 0.5 : 1.0;
        rayleigh_mie_sum += rayleigh_mie_i * weight_i;
    }
    return rayleigh_mie_sum;
}

float3 extrapolated_single_mie(float4 scattering) {
    if(scattering.r == 0.0) {
        return float3(0, 0, 0);
    }
    return scattering.rgb * scattering.a / scattering.r
        * (rayleigh_scattering.r / mie_scattering.r)
        * (mie_scattering / rayleigh_scattering);
}

float3 combined_scattering(AtmosphereParams atmo, 
    Sampler3D scattering,
    float r, float mu, float mu_s, float nu,
    bool ray_intersects_ground,
    out float3 single_mie)
{
    if(ray_intersects_ground) {
        single_mie = float3(0, 0, 0);
        return float3(0, 0, 0);
    }

    const uint3 size = image_size(scattering);
    const float4 uvwz = scattering_uvwz_from_r_mu_mu_s_nu(atmo, r, mu, mu_s, nu, ray_intersects_ground, size);
    const float tex_coord_x = uvwz.x * float(scattering_nu_size - 1);
    const float tex_x = floor(tex_coord_x);
    const float blend_t = tex_coord_x - tex_x;
    const float3 uvw0 = float3((tex_x + uvwz.y) / float(scattering_nu_size), uvwz.z, uvwz.w);
    const float3 uvw1 = float3((tex_x + 1.0 + uvwz.y) / float(scattering_nu_size), uvwz.z, uvwz.w);

    const float4 combined =
        scattering.SampleLevel(uvw0, 0) * (1.0 - blend_t) +
        scattering.SampleLevel(uvw1, 0) * blend_t;
    single_mie = extrapolated_single_mie(combined);
    return combined.rgb;
}

}

float rayleigh_phase(float nu) {
    const float k = 3.0 / (16.0 * constants.pi);
    return k * (1.0 + nu * nu);
}

float3 compute_transmittance_lut(AtmosphereParams atmo, float2 frag_coord, uint2 size) {
    float r, mu;
    atmosphere::r_mu_from_transmittance_uv(atmo, frag_coord / float2(size), size, r, mu);
    return atmosphere::transmittance_to_top(atmo, r, mu);
}

float3 compute_direct_irradiance_lut(AtmosphereParams atmo, Sampler2D transmittance, float2 frag_coord, uint2 size) {
    float r, mu_s;
    atmosphere::r_mu_s_from_irradiance_uv(atmo, frag_coord / float2(size), size, r, mu_s);
    return atmosphere::direct_irradiance(atmo, transmittance, r, mu_s);
}

void compute_single_scattering_lut(AtmosphereParams atmo, 
    Sampler2D transmittance, float3 frag_coord, uint3 size,
    out float3 rayleigh, out float3 mie)
{
    float r, mu, mu_s, nu;
    bool ray_intersects_ground;
    atmosphere::r_mu_mu_s_nu_from_scattering_frag_coord(atmo, 
        frag_coord, size, r, mu, mu_s, nu, ray_intersects_ground);
    atmosphere::single_scattering(atmo, 
        transmittance, r, mu, mu_s, nu, ray_intersects_ground, rayleigh, mie);
}

float3 compute_scattering_density_lut(AtmosphereParams atmo, 
    Sampler2D transmittance,
    Sampler3D delta_rayleigh,
    Sampler3D delta_mie,
    Sampler3D delta_multiple,
    Sampler2D delta_irradiance,
    float3 frag_coord,
    uint3 size,
    int scattering_order)
{
    float r, mu, mu_s, nu;
    bool ray_intersects_ground;
    atmosphere::r_mu_mu_s_nu_from_scattering_frag_coord(atmo, 
        frag_coord, size, r, mu, mu_s, nu, ray_intersects_ground);
    return atmosphere::scattering_density(atmo, 
        transmittance, delta_rayleigh, delta_mie, delta_multiple, delta_irradiance,
        r, mu, mu_s, nu, scattering_order);
}

float3 compute_indirect_irradiance_lut(AtmosphereParams atmo, 
    Sampler3D delta_rayleigh,
    Sampler3D delta_mie,
    Sampler3D delta_multiple,
    float2 frag_coord,
    uint2 size,
    int scattering_order)
{
    float r, mu_s;
    atmosphere::r_mu_s_from_irradiance_uv(atmo, frag_coord / float2(size), size, r, mu_s);
    return atmosphere::indirect_irradiance(atmo, 
        delta_rayleigh, delta_mie, delta_multiple, r, mu_s, scattering_order);
}

float3 compute_multiple_scattering_lut(AtmosphereParams atmo, 
    Sampler2D transmittance,
    Sampler3D delta_density,
    float3 frag_coord,
    uint3 size,
    out float nu)
{
    float r, mu, mu_s;
    bool ray_intersects_ground;
    atmosphere::r_mu_mu_s_nu_from_scattering_frag_coord(atmo, 
        frag_coord, size, r, mu, mu_s, nu, ray_intersects_ground);
    return atmosphere::multiple_scattering(atmo, 
        transmittance, delta_density, r, mu, mu_s, nu, ray_intersects_ground);
}

float3 sky_radiance(AtmosphereParams atmo, 
    Sampler2D transmittance,
    Sampler3D scattering,
    float3 camera,
    float3 view_ray,
    float3 sun_direction,
    out float3 transmittance_out)
{
    float r = length(camera);
    float rmu = dot(camera, view_ray);
    float distance_to_top = -rmu - sqrt(rmu * rmu - r * r + atmo.top_radius * atmo.top_radius);

    if(distance_to_top > 0.0) {
        camera = camera + view_ray * distance_to_top;
        r = atmo.top_radius;
        rmu += distance_to_top;
    } else if(r > atmo.top_radius) {
        transmittance_out = float3(1, 1, 1);
        return float3(0, 0, 0);
    }

    const float mu = rmu / r;
    const float mu_s = dot(camera, sun_direction) / r;
    const float nu = dot(view_ray, sun_direction);
    const bool ray_intersects_ground = atmosphere::intersects_ground(atmo, r, mu);

    transmittance_out = ray_intersects_ground
        ? float3(0, 0, 0)
        : atmosphere::sample_transmittance_to_top(atmo, transmittance, r, mu);

    float3 single_mie;
    const float3 scattering_rgb = atmosphere::combined_scattering(atmo, 
        scattering, r, mu, mu_s, nu, ray_intersects_ground, single_mie);

    return scattering_rgb * rayleigh_phase(nu)
         + single_mie * atmosphere::mie_phase(atmosphere::mie_phase_g, nu);
}

float3 sky_radiance_to_point(AtmosphereParams atmo, 
    Sampler2D transmittance,
    Sampler3D scattering,
    float3 camera,
    float3 point,
    float3 sun_direction,
    out float3 transmittance_out)
{
    const float3 view_ray = normalize(point - camera);
    float r = length(camera);
    float rmu = dot(camera, view_ray);
    float distance_to_top = -rmu - sqrt(rmu * rmu - r * r + atmo.top_radius * atmo.top_radius);

    if(distance_to_top > 0.0) {
        camera = camera + view_ray * distance_to_top;
        r = atmo.top_radius;
        rmu += distance_to_top;
    }

    const float mu = rmu / r;
    const float mu_s = dot(camera, sun_direction) / r;
    const float nu = dot(view_ray, sun_direction);
    const float d = length(point - camera);
    const bool ray_intersects_ground = atmosphere::intersects_ground(atmo, r, mu);

    transmittance_out = atmosphere::sample_transmittance(atmo, 
        transmittance, r, mu, d, ray_intersects_ground);

    float3 single_mie;
    float3 scattering_rgb = atmosphere::combined_scattering(atmo, 
        scattering, r, mu, mu_s, nu, ray_intersects_ground, single_mie);

    const float r_p = atmosphere::clamp_radius(atmo, sqrt(d * d + 2.0 * r * mu * d + r * r));
    const float mu_p = (r * mu + d) / r_p;
    const float mu_s_p = (r * mu_s + d * nu) / r_p;

    float3 single_mie_p;
    const float3 scattering_p = atmosphere::combined_scattering(atmo, 
        scattering, r_p, mu_p, mu_s_p, nu, ray_intersects_ground, single_mie_p);

    scattering_rgb = scattering_rgb - transmittance_out * scattering_p;
    single_mie = single_mie - transmittance_out * single_mie_p;
    single_mie = atmosphere::extrapolated_single_mie(float4(scattering_rgb, single_mie.r));
    single_mie = single_mie * smoothstep(0.0, 0.01, mu_s);

    return scattering_rgb * rayleigh_phase(nu)
         + single_mie * atmosphere::mie_phase(atmosphere::mie_phase_g, nu);
}


#endif // ATMOSPHERE_LIB_SLANG
