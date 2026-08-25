#ifndef SH_SLANG
#define SH_SLANG

#include "utils.slang"

// 2nd order spherical harmonics (9 coefficients).
struct SH {
    float Y00;
    float Y11;
    float Y10;
    float Y1_1;
    float Y21;
    float Y2_1;
    float Y2_2;
    float Y20;
    float Y22;

    __init() {
        Y00 = 0.0;
        Y11 = 0.0;
        Y10 = 0.0;
        Y1_1 = 0.0;
        Y21 = 0.0;
        Y2_1 = 0.0;
        Y2_2 = 0.0;
        Y20 = 0.0;
        Y22 = 0.0;
    }

    __init(float y00, float y11, float y10, float y1_1, float y21, float y2_1, float y2_2, float y20, float y22) {
        Y00 = y00;
        Y11 = y11;
        Y10 = y10;
        Y1_1 = y1_1;
        Y21 = y21;
        Y2_1 = y2_1;
        Y2_2 = y2_2;
        Y20 = y20;
        Y22 = y22;
    }

    static SH basis(float3 dir) {
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

    float eval(float3 dir) {
        const SH b = basis(normalize(dir));
        return Y00  * b.Y00
             + Y1_1 * b.Y1_1
             + Y10  * b.Y10
             + Y11  * b.Y11
             + Y2_2 * b.Y2_2
             + Y2_1 * b.Y2_1
             + Y20  * b.Y20
             + Y21  * b.Y21
             + Y22  * b.Y22
         ;
    }

    float eval_hemisphere_cosine_weighted(float3 dir) {
        const SH b = basis(dir);
        const float a0 = constants.pi;
        const float a1 = 2.0 / 3.0 * constants.pi;
        const float a2 = constants.pi / 4.0;
        return 
            (b.Y00 * Y00) * a0+ (b.Y1_1 * Y1_1 + b.Y10 * Y10 + b.Y11 * Y11) * a1 +
            (b.Y2_2 * Y2_2 + b.Y2_1 * Y2_1 + b.Y20 * Y20 + b.Y21 * Y21 + b.Y22 * Y22) * a2
        ;
    }

    static const uint sh_l0_bits = 16; // 16
    static const uint sh_l1_bits = 8;  // 8
    static const uint sh_l2_bits = 4;  // 4

    uint2 pack() {
        static_assert(sh_l0_bits + 2 * sh_l1_bits <= 32, "Invalid packing bit count");
        static_assert(sh_l1_bits + 5 * sh_l2_bits <= 32, "Invalid packing bit count");

        const float s = 2.0;
        const uint q00 = uint(saturate(Y00 / s * 0.5 + 0.5) * float((1u << sh_l0_bits) - 1) + 0.5);
        const uint q11 = uint(saturate(Y11 / s * 0.5 + 0.5) * float((1u << sh_l1_bits) - 1) + 0.5);
        const uint q10 = uint(saturate(Y10 / s * 0.5 + 0.5) * float((1u << sh_l1_bits) - 1) + 0.5);

        const uint q1_1 = uint(saturate(Y1_1 / s * 0.5 + 0.5) * float((1u << sh_l1_bits) - 1) + 0.5);

        const uint q21 = uint(saturate(Y21 / s * 0.5 + 0.5) * float((1u << sh_l2_bits) - 1) + 0.5);
        const uint q2_1 = uint(saturate(Y2_1 / s * 0.5 + 0.5) * float((1u << sh_l2_bits) - 1) + 0.5);
        const uint q2_2 = uint(saturate(Y2_2 / s * 0.5 + 0.5) * float((1u << sh_l2_bits) - 1) + 0.5);
        const uint q20 = uint(saturate(Y20 / s * 0.5 + 0.5) * float((1u << sh_l2_bits) - 1) + 0.5);
        const uint q22 = uint(saturate(Y22 / s * 0.5 + 0.5) * float((1u << sh_l2_bits) - 1) + 0.5);

        const uint Q0 = 
            (q00 << (2 * sh_l1_bits)) | 
            (q11 << sh_l1_bits) | 
            q10
        ;

        const uint Q1 = 
            (q1_1 << (2 * sh_l1_bits + 2 * sh_l2_bits)) | 
            (q21 << (2 * sh_l1_bits + sh_l2_bits)) | 
            (q2_1 << (2 * sh_l1_bits)) | 
            (q2_2 << (sh_l1_bits + sh_l2_bits)) | 
            (q20 << sh_l1_bits) | 
            (q22 << sh_l2_bits)
        ;

        return uint2(Q0, Q1);
    }

    static SH unpack(uint2 p) {
        const float s = 2.0;
        return SH(
            (float(p.x >> (2 * sh_l1_bits)) / float((1u << sh_l0_bits) - 1) * 2.0 - 1.0) * s,
            (float((p.x >> sh_l1_bits) & ((1u << sh_l1_bits) - 1)) / float((1u << sh_l1_bits) - 1) * 2.0 - 1.0) * s,
            (float(p.x & ((1u << sh_l1_bits) - 1)) / float((1u << sh_l1_bits) - 1) * 2.0 - 1.0) * s,
            (float(p.y >> (2 * sh_l1_bits + 2 * sh_l2_bits)) / float((1u << sh_l1_bits) - 1) * 2.0 - 1.0) * s,
            (float((p.y >> (2 * sh_l1_bits + sh_l2_bits)) & ((1u << sh_l2_bits) - 1)) / float((1u << sh_l2_bits) - 1) * 2.0 - 1.0) * s,
            (float((p.y >> (2 * sh_l1_bits)) & ((1u << sh_l2_bits) - 1)) / float((1u << sh_l2_bits) - 1) * 2.0 - 1.0) * s,
            (float((p.y >> (sh_l1_bits + sh_l2_bits)) & ((1u << sh_l2_bits) - 1)) / float((1u << sh_l2_bits) - 1) * 2.0 - 1.0) * s,
            (float((p.y >> sh_l1_bits) & ((1u << sh_l2_bits) - 1)) / float((1u << sh_l2_bits) - 1) * 2.0 - 1.0) * s,
            (float((p.y >> sh_l2_bits) & ((1u << sh_l2_bits) - 1)) / float((1u << sh_l2_bits) - 1) * 2.0 - 1.0) * s
        );
    }
};


bool isnan(SH sh) {
    return 
        isnan(sh.Y00) || isnan(sh.Y11) || isnan(sh.Y10) || isnan(sh.Y1_1) ||
        isnan(sh.Y21) || isnan(sh.Y2_1) || isnan(sh.Y2_2) || isnan(sh.Y20) || isnan(sh.Y22)
   ;
}

SH operator+(SH a, SH b) {
    return SH(
        a.Y00 + b.Y00,
        a.Y11 + b.Y11,
        a.Y10 + b.Y10,
        a.Y1_1 + b.Y1_1,
        a.Y21 + b.Y21,
        a.Y2_1 + b.Y2_1,
        a.Y2_2 + b.Y2_2,
        a.Y20 + b.Y20,
        a.Y22 + b.Y22
    );
}

SH operator-(SH a, SH b) {
    return SH(
        a.Y00 - b.Y00,
        a.Y11 - b.Y11,
        a.Y10 - b.Y10,
        a.Y1_1 - b.Y1_1,
        a.Y21 - b.Y21,
        a.Y2_1 - b.Y2_1,
        a.Y2_2 - b.Y2_2,
        a.Y20 - b.Y20,
        a.Y22 - b.Y22
    );
}

SH operator*(SH a, float s) {
    return SH(
        a.Y00 * s,
        a.Y11 * s,
        a.Y10 * s,
        a.Y1_1 * s,
        a.Y21 * s,
        a.Y2_1 * s,
        a.Y2_2 * s,
        a.Y20 * s,
        a.Y22 * s
    );
}

SH operator/(SH a, float s) {
    return a * (1.0 / s);
}

#endif // SH_SLANG
