#include "lib/utils.slang"
#include "lib/atmosphere.slang"


#ifdef __cplusplus
#define SINGLE
#define MULTIPLE
#endif


#ifdef SINGLE
[[vk::binding(0)]]
RWTexture2D<float4> out_transmittance;

[[vk::binding(1)]]
RWTexture2D<float4> out_delta_irradiance;

[[vk::binding(2)]]
Sampler2D in_transmittance;

[[vk::binding(3)]]
RWTexture3D<float4> out_delta_rayleigh;

[[vk::binding(4)]]
RWTexture3D<float4> out_delta_mie;

[[vk::binding(5)]]
RWTexture3D<float4> out_scattering;

[[vk::binding(6)]]
ConstantBuffer<AtmosphereParams> atmos_params;

[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const uint3 coord = semantics.global_id;
    const uint3 scattering_size = image_size(out_scattering);
    const uint2 transmittance_size = image_size(out_transmittance);
    const uint2 irradiance_size = image_size(out_delta_irradiance);

    if(coord.z == 0) {
        if(all(coord.xy < transmittance_size)) {
            const float2 frag_coord = float2(coord.xy) + 0.5;
            out_transmittance[coord.xy] = float4(
                compute_transmittance_lut(atmos_params, frag_coord, transmittance_size), 1.0);
        }

        if(all(coord.xy < irradiance_size)) {
            const float2 frag_coord = float2(coord.xy) + 0.5;
            out_delta_irradiance[coord.xy] = float4(
                compute_direct_irradiance_lut(atmos_params, in_transmittance, frag_coord, irradiance_size), 1.0);
        }
    }

    if(any(coord >= scattering_size)) {
        return;
    }

    const float3 frag_coord = float3(coord) + 0.5;

    float3 delta_rayleigh;
    float3 delta_mie;
    compute_single_scattering_lut(
        atmos_params, in_transmittance, frag_coord, scattering_size, delta_rayleigh, delta_mie);

    out_delta_rayleigh[coord] = float4(delta_rayleigh, 1.0);
    out_delta_mie[coord] = float4(delta_mie, 1.0);
    out_scattering[coord] = float4(delta_rayleigh, delta_mie.r);
}
#endif


#ifdef MULTIPLE
[[vk::binding(0)]]
Sampler2D in_transmittance;

[[vk::binding(1)]]
Sampler3D in_delta_rayleigh;

[[vk::binding(2)]]
Sampler3D in_delta_mie;

[[vk::binding(3)]]
Sampler3D in_delta_multiple;

[[vk::binding(4)]]
Sampler2D in_delta_irradiance;

[[vk::binding(5)]]
RWTexture3D<float4> out_delta_density;

[[vk::binding(6)]]
Sampler3D in_delta_density;

[[vk::binding(7)]]
RWTexture2D<float4> out_delta_irradiance;

[[vk::binding(8)]]
RWTexture3D<float4> out_delta_multiple;

[[vk::binding(9)]]
RWTexture3D<float4> out_scattering;

[[vk::binding(10)]]
Sampler3D in_scattering;

[[vk::binding(11)]]
ConstantBuffer<AtmosphereParams> atmos_params;

[[vk::binding(12)]]
cbuffer Params {
    uint scattering_order;
    uint max_scattering_order;
};

[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const uint3 coord = semantics.global_id;
    const uint3 scattering_size = image_size(out_scattering);
    const uint2 irradiance_size = image_size(out_delta_irradiance);

    if(any(coord >= scattering_size)) {
        return;
    }

    const float3 frag_coord = float3(coord) + 0.5;
    const int order = int(scattering_order);

    const float3 density = compute_scattering_density_lut(
        atmos_params,
        in_transmittance,
        in_delta_rayleigh,
        in_delta_mie,
        in_delta_multiple,
        in_delta_irradiance,
        frag_coord,
        scattering_size,
        order);

    out_delta_density[coord] = float4(density, 1.0);

    if(coord.z == 0 && all(coord.xy < irradiance_size) && scattering_order < max_scattering_order) {
        const float2 irr_coord = float2(coord.xy) + 0.5;
        out_delta_irradiance[coord.xy] = float4(
            compute_indirect_irradiance_lut(
                atmos_params,
                in_delta_rayleigh,
                in_delta_mie,
                in_delta_multiple,
                irr_coord,
                irradiance_size,
                order - 1),
            1.0);
    }

    float nu;
    const float3 delta_multiple = compute_multiple_scattering_lut(
        atmos_params,
        in_transmittance,
        in_delta_density,
        frag_coord,
        scattering_size,
        nu);

    out_delta_multiple[coord] = float4(delta_multiple, 1.0);

    const float3 uvw = (float3(coord) + 0.5) / float3(scattering_size);
    const float4 prev = in_scattering.SampleLevel(uvw, 0);
    out_scattering[coord] = float4(prev.rgb + delta_multiple / rayleigh_phase(nu), prev.a);
}
#endif
