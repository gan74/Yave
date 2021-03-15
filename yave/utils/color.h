/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_UTILS_COLOR_H
#define YAVE_UTILS_COLOR_H

#include <yave/yave.h>

namespace yave {

// https://docs.unrealengine.com/en-US/Engine/Rendering/PostProcessEffects/AutomaticExposure/index.html
inline float EV100_to_exposure(float ev) {
    return 1.0f / std::pow(2.0f, ev);
}

inline float exposure_to_EV100(float exposure) {
    return -std::log(std::max(math::epsilon<float>, exposure)) / std::log(2.0f);
}


inline float sRGB_to_linear(float x) {
    if(x <= 0.04045f) {
        return x / 12.92f;
    }
    return std::pow((x + 0.055f) / 1.055f, 2.4f);
}

inline float linear_to_sRGB(float x) {
    if(x <= 0.0031308f) {
        return x * 12.92f;
    }
    return 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
}

inline math::Vec3 sRGB_to_linear(const math::Vec3& v) {
    return math::Vec3(sRGB_to_linear(v.x()), sRGB_to_linear(v.y()), sRGB_to_linear(v.z()));
}

inline math::Vec3 linear_to_sRGB(const math::Vec3& v) {
    return math::Vec3(linear_to_sRGB(v.x()), linear_to_sRGB(v.y()), linear_to_sRGB(v.z()));
}

inline math::Vec4 sRGB_to_linear(const math::Vec4& v) {
    return math::Vec4(sRGB_to_linear(v.to<3>()), v.w());
}

inline math::Vec4 linear_to_sRGB(const math::Vec4& v) {
    return math::Vec4(linear_to_sRGB(v.to<3>()), v.w());
}


// from imgui
inline math::Vec3 hsv_to_rgb(float h, float s, float v) {
    h = std::fmod(h, 1.0f) * 6.0f;
    const int i = int(h);
    const float f = h - float(i);
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - s * f);
    const float t = v * (1.0f - s * (1.0f - f));

    switch(i) {
        case 0: return math::Vec3(v, t, p);
        case 1: return math::Vec3(q, v, p);
        case 2: return math::Vec3(p, v, t);
        case 3: return math::Vec3(p, q, v);
        case 4: return math::Vec3(t, p, v);

        default:
        break;
    }
    return math::Vec3(v, p, q);
}

// http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
inline math::Vec3 k_to_rbg(float k) {
    const double k_100 = std::min(40000.0, std::max(1000.0, double(k))) / 100.0;
    const double r = k_100 <= 66.0
        ? 255.0
        : 329.698727446 * std::pow(k_100 - 60.0, -0.1332047592);
    const double g = k_100 <= 66.0
        ? 99.4708025861 * std::log(k_100) - 161.1195681661
        : 288.1221695283 * std::pow(k_100 - 60.0, -0.0755148492);

    const double b = k_100 >= 66.0
        ? 255.0
        : k_100 <= 19.0
            ? 0.0
            : 138.5177312231 * std::log(k_100 - 10) - 305.0447927307;

    return (math::Vec3(float(r), float(g), float(b)) / 255.0f).saturated();
}

inline float rgb_to_k(const math::Vec3& rgb) {
    if(rgb.x() >= 1.0f) {
        const double g = double(rgb.y()) * 255.0;
        const double k_100 = 5.0519153526 * std::exp(0.0100532012812 * g);
        return float(k_100 * 100.0);
    }

    const double r = double(rgb.x()) * 255.0;
    const double x = std::pow(r, 7.507239275877164);
    const double k_100 = (8.018790685011271e18 + 60.0 * x) / x;
    return float(k_100 * 100.0);
}

// https://gamedev.stackexchange.com/a/46469
inline math::Vec3 identifying_color(usize index, float s = 0.5f, float v = 1.0f) {
    return hsv_to_rgb(std::fmod(index * 0.618033988749895f, 1.0f), s, v);
}

}

#endif // YAVE_UTILS_COLOR_H

