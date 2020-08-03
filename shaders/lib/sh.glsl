#ifndef SH_GLSL
#define SH_GLSL

#include "constants.glsl"

// https://seblagarde.wordpress.com/2011/10/09/dive-in-sh-buffer-idea/
// https://www.shadertoy.com/view/lt2GRD
// https://xlgames-inc.github.io/posts/sphericalharmonics2/
// http://silviojemma.com/public/papers/lighting/spherical-harmonic-lighting.pdf
// https://zvxryb.github.io/blog/2015/09/03/sh-lighting-part2/
// https://solid-angle.blogspot.com/2009/12/screen-space-spherical-harmonic.html
// https://computergraphics.stackexchange.com/questions/4997/spherical-harmonics-diffuse-cubemap-how-to-get-coefficients

struct SHBasis {
    float Y00, Y11, Y10, Y1_1, Y21, Y2_1, Y2_2, Y20, Y22;
};

struct SH {
    // vec4 for padding
    vec4 L00, L11, L10, L1_1, L21, L2_1, L2_2, L20, L22;
};



SH empty_sh() {
    const vec4 z = vec4(0.0, 0.0, 0.0, 0.0);
    return SH(z, z, z, z, z, z, z, z, z);
}

SHBasis compute_sh_basis(vec3 dir) {
    return SHBasis(
        0.282095,
        0.488603 * dir.x,
        0.488603 * dir.z,
        0.488603 * dir.y,
        1.092548 * dir.x * dir.z,
        1.092548 * dir.y * dir.z,
        1.092548 * dir.y * dir.x,
        0.946176 * dir.z * dir.z - 0.315392,
        0.546274 * (dir.x * dir.x - dir.y * dir.y)
    );
}

SH compute_sh(vec3 col, vec3 dir) {
    const vec4 color = vec4(col, 0.0);
    SHBasis basis = compute_sh_basis(dir);
    return SH(
        basis.Y00 * color,
        basis.Y11 * color,
        basis.Y10 * color,
        basis.Y1_1 * color,
        basis.Y21 * color,
        basis.Y2_1 * color,
        basis.Y2_2 * color,
        basis.Y20 * color,
        basis.Y22 * color
    );
}

SH add_sh(SH a, SH b) {
    return SH(
        b.L00 + a.L00,
        b.L11 + a.L11,
        b.L10 + a.L10,
        b.L1_1 + a.L1_1,
        b.L21 + a.L21,
        b.L2_1 + a.L2_1,
        b.L2_2 + a.L2_2,
        b.L20 + a.L20,
        b.L22 + a.L22
    );
}

SH mul_sh(SH sh, float d) {
    return SH(
        sh.L00 * d,
        sh.L11 * d,
        sh.L10 * d,
        sh.L1_1 * d,
        sh.L21 * d,
        sh.L2_1 * d,
        sh.L2_2 * d,
        sh.L20 * d,
        sh.L22 * d
    );
}

vec3 eval_sh(SH sh, vec3 dir) {
    const vec3 A = vec3(pi, 2.0 / 3.0 * pi, pi / 4.0);
    const SHBasis basis = compute_sh_basis(dir);
    return
        A.x * basis.Y00  * sh.L00.xyz +
        A.y * basis.Y1_1 * sh.L1_1.xyz +
        A.y * basis.Y10  * sh.L10.xyz +
        A.y * basis.Y11  * sh.L11.xyz +
        A.z * basis.Y2_2 * sh.L2_2.xyz +
        A.z * basis.Y2_1 * sh.L2_1.xyz +
        A.z * basis.Y20  * sh.L20.xyz +
        A.z * basis.Y21  * sh.L21.xyz +
        A.z * basis.Y22  * sh.L22.xyz;
}


#endif

