#ifndef VPL_GLSL
#define VPL_GLSL

#include "utils.glsl"


struct VPL {
    vec3 position;
    uint padding_0;

    vec3 color;
    uint padding_1;

    vec3 normal;
    uint padding_2;
};

#endif

