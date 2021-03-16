#ifndef COMPUTE_DERIVATIVES_GLSL
#define COMPUTE_DERIVATIVES_GLSL

#extension GL_KHR_shader_subgroup_quad: enable

#define fwidth quad_fwidth




#define GEN_FWIDTH(type)                                                \
type quad_fwidth(type x) {                                              \
    const type dx = abs(x - subgroupQuadSwapHorizontal(x));             \
    const type dy = abs(x - subgroupQuadSwapHorizontal(x));             \
    return dx + dy;                                                     \
}

GEN_FWIDTH(float)
GEN_FWIDTH(vec2)
GEN_FWIDTH(vec3)
GEN_FWIDTH(vec4)

#undef GEN_FWIDTH

#endif

