#ifndef COMPUTE_DERIVATIVES_GLSL
#define COMPUTE_DERIVATIVES_GLSL

#extension GL_KHR_shader_subgroup_quad: enable

#define fwidth quad_fwidth
#define dFdx quad_dfdx
#define dFdy quad_dfdy




#define GEN_DERIVATIVES(type)                                           \
type quad_fwidth(type x) {                                              \
    const type dx = abs(x - subgroupQuadSwapHorizontal(x));             \
    const type dy = abs(x - subgroupQuadSwapVertical(x));               \
    return (dx + dy);                                                   \
}                                                                       \
type quad_dfdx(type x) {                                                \
    return (x - subgroupQuadSwapHorizontal(x));                         \
}                                                                       \
type quad_dfdy(type x) {                                                \
    return (x - subgroupQuadSwapVertical(x));                           \
}


GEN_DERIVATIVES(float)
GEN_DERIVATIVES(vec2)
GEN_DERIVATIVES(vec3)
GEN_DERIVATIVES(vec4)

#undef GEN_FWIDTH

#endif

