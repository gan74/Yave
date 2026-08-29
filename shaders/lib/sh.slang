#ifndef SH_SLANG
#define SH_SLANG

#include "utils.slang"

// 2nd order spherical harmonics (9 coefficients).
struct SH<T : IFloat> {
    T Y00;
    T Y11;
    T Y10;
    T Y1_1;
    T Y21;
    T Y2_1;
    T Y2_2;
    T Y20;
    T Y22;

    __init() {
        Y00 = T(0.0);
        Y11 = T(0.0);
        Y10 = T(0.0);
        Y1_1 = T(0.0);
        Y21 = T(0.0);
        Y2_1 = T(0.0);
        Y2_2 = T(0.0);
        Y20 = T(0.0);
        Y22 = T(0.0);
    }

    __init(T y00, T y11, T y10, T y1_1, T y21, T y2_1, T y2_2, T y20, T y22) {
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

    static SH<float> basis(float3 dir) {
        return SH<float>(
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

    T eval(float3 dir) {
        const SH<float> b = basis(normalize(dir));
        return Y00  * T(b.Y00)
             + Y1_1 * T(b.Y1_1)
             + Y10  * T(b.Y10)
             + Y11  * T(b.Y11)
             + Y2_2 * T(b.Y2_2)
             + Y2_1 * T(b.Y2_1)
             + Y20  * T(b.Y20)
             + Y21  * T(b.Y21)
             + Y22  * T(b.Y22)
         ;
    }

    T eval_hemisphere_cosine_weighted(float3 dir) {
        const SH<float> b = basis(dir);
        const float a0 = constants.pi;
        const float a1 = 2.0 / 3.0 * constants.pi;
        const float a2 = constants.pi / 4.0;
        return
            (T(b.Y00) * Y00) * T(a0) + (T(b.Y1_1) * Y1_1 + T(b.Y10) * Y10 + T(b.Y11) * Y11) * T(a1) +
            (T(b.Y2_2) * Y2_2 + T(b.Y2_1) * Y2_1 + T(b.Y20) * Y20 + T(b.Y21) * Y21 + T(b.Y22) * Y22) * T(a2)
        ;
    }
};


extension<int N, T : __BuiltinFloatingPointType> SH<vector<T, N>> {
    __subscript(int index) -> SH<T> {
        get {
            return SH<T>(
                Y00[index],
                Y11[index],
                Y10[index],
                Y1_1[index],
                Y21[index],
                Y2_1[index],
                Y2_2[index],
                Y20[index],
                Y22[index]
            );
        }
        set {
            Y00[index] = newValue.Y00;
            Y11[index] = newValue.Y11;
            Y10[index] = newValue.Y10;
            Y1_1[index] = newValue.Y1_1;
            Y21[index] = newValue.Y21;
            Y2_1[index] = newValue.Y2_1;
            Y2_2[index] = newValue.Y2_2;
            Y20[index] = newValue.Y20;
            Y22[index] = newValue.Y22;
        }
    }
}

extension SH<float> {
    float3 strongest_dir() {
        float3 dir = float3(Y11, Y1_1, Y10);
        if(dot(dir, dir) < constants.epsilon) {
            dir = float3(0.0, 0.0, 1.0);
        } else {
            dir = normalize(dir);
        }

        for(uint i = 0; i != 4; ++i) {
            const float3 grad = float3(
                0.488603 * Y11 + 1.092548 * (Y21 * dir.z + Y2_2 * dir.y + Y22 * dir.x),
                0.488603 * Y1_1 + 1.092548 * (Y2_1 * dir.z + Y2_2 * dir.x - Y22 * dir.y),
                0.488603 * Y10 + 1.092548 * (Y21 * dir.x + Y2_1 * dir.y) + 1.892352 * Y20 * dir.z
            );
            if(dot(grad, grad) < constants.epsilon) {
                break;
            }
            dir = normalize(grad);
        }

        return eval(-dir) > eval(dir) ? -dir : dir;
    }

    private static uint sh_to_unorm(float v, float scale, uint bits) {
        const float max_val = float((1u << bits) - 1);
        return uint(saturate(v / scale * 0.5 + 0.5) * max_val + 0.5);
    }

    private static float sh_from_unorm(uint q, float scale, uint bits) {
        const float max_val = float((1u << bits) - 1);
        return (float(q) / max_val * 2.0 - 1.0) * scale;
    }

    uint4 pack() {
        const float scale = max(
            max(max(abs(Y00), abs(Y11)), max(abs(Y10), abs(Y1_1))),
            max(max(abs(Y21), abs(Y2_1)), max(abs(Y2_2), max(abs(Y20), abs(Y22))))
        );

        return uint4(
            (sh_to_unorm(Y00,  scale, 11) << 21) | (sh_to_unorm(Y11,  scale, 11) << 10) | sh_to_unorm(Y10,  scale, 10),
            (sh_to_unorm(Y1_1, scale, 11) << 21) | (sh_to_unorm(Y21,  scale, 11) << 10) | sh_to_unorm(Y2_1, scale, 10),
            (sh_to_unorm(Y2_2, scale, 11) << 21) | (sh_to_unorm(Y20,  scale, 11) << 10) | sh_to_unorm(Y22,  scale, 10),
            asuint(scale)
        );
    }

    static SH<float> unpack(uint4 p) {
        const float scale = asfloat(p.w);
        return SH<float>(
            sh_from_unorm(p.x >> 21,            scale, 11),
            sh_from_unorm((p.x >> 10) & 0x7FF,  scale, 11),
            sh_from_unorm(p.x & 0x3FF,          scale, 10),
            sh_from_unorm(p.y >> 21,            scale, 11),
            sh_from_unorm((p.y >> 10) & 0x7FF,  scale, 11),
            sh_from_unorm(p.y & 0x3FF,          scale, 10),
            sh_from_unorm(p.z >> 21,            scale, 11),
            sh_from_unorm((p.z >> 10) & 0x7FF,  scale, 11),
            sh_from_unorm(p.z & 0x3FF,          scale, 10)
        );
    }
}


bool isnan<T>(SH<T> sh) where T : IFloat, __BuiltinFloatingPointType {
    return
        isnan(sh.Y00) || isnan(sh.Y11) || isnan(sh.Y10) || isnan(sh.Y1_1) ||
        isnan(sh.Y21) || isnan(sh.Y2_1) || isnan(sh.Y2_2) || isnan(sh.Y20) || isnan(sh.Y22)
   ;
}

SH<T> operator+<T : IFloat>(SH<T> a, SH<T> b) {
    return SH<T>(
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

SH<T> operator-<T : IFloat>(SH<T> a, SH<T> b) {
    return SH<T>(
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

SH<T> operator*<T : IFloat>(SH<T> a, T s) {
    return SH<T>(
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


#endif // SH_SLANG
