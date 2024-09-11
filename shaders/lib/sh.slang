#ifndef SH_SLANG
#define SH_SLANG

#include "utils.slang"

// https://github.com/sebh/HLSL-Spherical-Harmonics/blob/master/SphericalHarmonics.hlsl
// https://seblagarde.wordpress.com/2011/10/09/dive-in-sh-buffer-idea/
// https://www.shadertoy.com/view/lt2GRD
// https://xlgames-inc.github.io/posts/sphericalharmonics2/
// http://silviojemma.com/public/papers/lighting/spherical-harmonic-lighting.pdf
// https://zvxryb.github.io/blog/2015/09/03/sh-lighting-part2/
// https://solid-angle.blogspot.com/2009/12/screen-space-spherical-harmonic.html
// https://computergraphics.stackexchange.com/questions/4997/spherical-harmonics-diffuse-cubemap-how-to-get-coefficients
// https://github.com/gan74/Yave/blob/y_ecs/shaders/lib/sh.glsl


struct SH : SHBase {
    __init() {
    }

    __init(SHBase sh) {
        Y00  = sh.Y00;
        Y11  = sh.Y11;
        Y10  = sh.Y10;
        Y1_1 = sh.Y1_1;
        Y21  = sh.Y21;
        Y2_1 = sh.Y2_1;
        Y2_2 = sh.Y2_2;
        Y20  = sh.Y20;
        Y22  = sh.Y22;
    }

    __init(float a, float b, float c, float d, float e, float f, float g, float h, float i) {
        Y00 = a;
        Y11 = b;
        Y10 = c;
        Y1_1 = d;
        Y21 = e;
        Y2_1 = f;
        Y2_2 = g;
        Y20 = h;
        Y22 = i;
    }

    static SH around(float3 dir) {
        return SH(
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

    float unproject(float3 dir) {
        const float3 A = float3(pi, 2.0 / 3.0 * pi, pi / 4.0);
        const SH basis = SH.around(dir);
        return (
            A.x * basis.Y00  * Y00 +
            A.y * basis.Y1_1 * Y1_1 +
            A.y * basis.Y10  * Y10 +
            A.y * basis.Y11  * Y11 +
            A.z * basis.Y2_2 * Y2_2 +
            A.z * basis.Y2_1 * Y2_1 +
            A.z * basis.Y20  * Y20 +
            A.z * basis.Y21  * Y21 +
            A.z * basis.Y22  * Y22
        );
    }
};





struct SHRgb {
    SH r, g, b;


    __init() {
    }

    __init(SH red, SH green, SH blue) {
        r = red;
        g = green;
        b = blue;
    }

    float3 unproject(float3 dir) {
        return float3(r.unproject(dir), g.unproject(dir), b.unproject(dir));
    }
};







SH operator+(SH a, SH b) {
    return SH(
        b.Y00  + a.Y00,
        b.Y11  + a.Y11,
        b.Y10  + a.Y10,
        b.Y1_1 + a.Y1_1,
        b.Y21  + a.Y21,
        b.Y2_1 + a.Y2_1,
        b.Y2_2 + a.Y2_2,
        b.Y20  + a.Y20,
        b.Y22  + a.Y22
    );
}

SH operator*(SH a, float b) {
    return SH(
        a.Y00  * b,
        a.Y11  * b,
        a.Y10  * b,
        a.Y1_1 * b,
        a.Y21  * b,
        a.Y2_1 * b,
        a.Y2_2 * b,
        a.Y20  * b,
        a.Y22  * b
    );
}

SHRgb operator*(SH a, float3 b) {
    return SHRgb(a * b.r, a * b.g, a * b.b);
}

SHRgb operator+(SHRgb a, SHRgb b) {
    return SHRgb(a.r + b.r, a.g + b.g, a.b + b.b);
}

SHRgb operator*(SHRgb a, float b) {
    return SHRgb(a.r * b, a.g * b, a.b * b);
}

#endif // SH

