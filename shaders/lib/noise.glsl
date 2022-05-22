#ifndef NOISE_GLSL
#define NOISE_GLSL

#include "utils.glsl"

// https://www.shadertoy.com/view/4djSRW
// https://www.shadertoy.com/view/llGSzw

uint hash_1_1(uint n)  {
    // integer hash copied from Hugo Elias
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return n;
}

float hash_1_1(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float hash_1_2(vec2 p) {
    vec3 p3  = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

#endif

