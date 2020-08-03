#ifndef HDR_GLSL
#define HDR_GLSL

#include "utils.glsl"

float avg_to_EV100(float avg) {
    return log2(avg * 100.0 / 12.5);
}

// We divide by 1.2 to avoid crushing spec too much
float EV100_to_exposure(float EV100) {
    float max_lum = 1.2 * pow(2.0, EV100);
    return 1.0 / max_lum;
}

vec3 expose_RGB(vec3 hdr, float exposure) {
    return hdr * exposure;
}

vec3 expose_Yxy(vec3 hdr, float exposure) {
    vec3 Yxy = RGB_to_Yxy(hdr);
    Yxy.x *= exposure;
    return Yxy_to_RGB(Yxy);
}

float uncharted2(float x) {
    float a = 0.22;
    float b = 0.30;
    float c = 0.10;
    float d = 0.20;
    float e = 0.01;
    float f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float ACES(float x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return ((x * (a * x + b)) / (x * (c * x + d) + e));
}


float reinhard(float hdr) {
    return hdr / (hdr + 1.0);
}




vec3 uncharted2(vec3 x) {
    return vec3(uncharted2(x.x), uncharted2(x.y), uncharted2(x.z));
}

vec3 ACES(vec3 x) {
    return vec3(ACES(x.x), ACES(x.y), ACES(x.z));
}

vec3 reinhard(vec3 x) {
    return vec3(reinhard(x.x), reinhard(x.y), reinhard(x.z));
}

#endif // HDR_GLSL

