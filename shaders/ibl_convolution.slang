#include "lib/utils.slang"


#ifdef CUBE
#define InputSampler CubeMap
#endif

#ifdef EQUIREC
#define InputSampler EquirecMap
#endif


[[vk::binding(0)]]
InputSampler in_envmap;

[[vk::binding(1)]]
RWTexture2DArray out_image;

[[vk::binding(2)]]
cbuffer Roughness_Inline {
    float roughness;
    float size;
};




float3 debug_cube(uint side) {
    switch(side) {
        case 0: return float3(1.0, 0.5, 0.5); // left
        case 1: return float3(0.0, 0.5, 0.5); // right
        case 2: return float3(0.5, 1.0, 0.5); // up
        case 3: return float3(0.5, 0.0, 0.5); // down
        case 4: return float3(0.5, 0.5, 1.0); // front
        case 5: return float3(0.5, 0.5, 0.0); // back
    }
    return 0.0;
}

float3 cube_dir(float2 tex_coord, uint side) {
    const float2 tex = tex_coord * 2.0 - 1.0;

    switch(side) {
        case 0: return float3(1.0, -tex.x, -tex.y);     // left
        case 1: return float3(-1.0, tex.x , -tex.y);    // right
        case 2: return float3(tex.x, tex.y , 1.0);      // up
        case 3: return float3(tex.x, -tex.y, -1.0);     // down
        case 4: return float3(tex.x, 1.0, -tex.y);      // front
        case 5: return float3(-tex.x, -1.0, -tex.y);    // back
    }

    return 0.0;
}


[shader("compute")]
[numthreads(8, 8, 1)]
void comp_main() {
    const uint2 coord = semantics.global_id.xy;
    const float2 uv = (semantics.global_id.xy + 0.5) / size;

    const float3 local_dir = normalize(cube_dir(uv, semantics.global_id.z));

    float3 conv;
    if(roughness <= 0.0) {
        conv = in_envmap.SampleCube(local_dir).rgb;
    } else if(roughness >= 1.0) {
        conv = diffuse_convolution(in_envmap, local_dir);
    } else {
        conv = specular_convolution(in_envmap, local_dir, roughness);
    }

    out_image[uint3(coord, semantics.global_id.z)] = float4(conv, roughness);
}

