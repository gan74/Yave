import "glsl";

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
};




float3 debug_cube(uint side) {
    switch(side) {
        case 0: return float3(1.0, 0.5, 0.5); // front
        case 1: return float3(0.0, 0.5, 0.5); // back
        case 2: return float3(0.5, 1.0, 0.5); // right
        case 3: return float3(0.5, 0.0, 0.5); // left
        case 4: return float3(0.5, 0.5, 1.0); // top
        case 5: return float3(0.5, 0.5, 0.0); // bottom
    }
    return 0.0;
}

float3 cube_dir(float2 tex_coord, uint side) {
    const float2 tex = tex_coord * 2.0 - 1.0;

    switch(side) {
        case 0: return float3(1.0, -tex.y, -tex.x); // front
        case 1: return float3(-1.0, -tex.y, tex.x); // back
        case 2: return float3(tex.x, 1.0, tex.y); // right
        case 3: return float3(tex.x, -1.0, -tex.y); // left
        case 4: return float3(tex.x, -tex.y, 1.0); // top
        case 5: return float3(-tex.x, -tex.y, -1.0); // bottom
    }

    return 0.0;
}


[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const int2 coord = int2(semantics.global_id.xy);
    const float2 uv = (semantics.global_id.xy + 0.5) / float2(image_size(out_image).xy);

    for(uint i = 0; i != 6; ++i) {
        const float3 local_dir = normalize(cube_dir(uv, i));

        float3 conv;
        if(roughness <= 0.0) {
            conv = in_envmap.SampleCube(local_dir).rgb;
        } else if(roughness >= 1.0) {
            conv = diffuse_convolution(in_envmap, local_dir);
        } else {
            conv = specular_convolution(in_envmap, local_dir, roughness);
        }

        out_image[int3(coord, i)] = float4(conv, 0.0);
    }
}

