/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

//https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
math::Vec3 hue_to_rgb(float h) {
	float h6 = h * 6.0f;
	float r = std::abs(h6 - 3.0f) - 1.0f;
	float g = 2.0f - std::abs(h6 - 2.0f);
	float b = 2.0f - std::abs(h6 - 4.0f);
	return {r, g, b};
}

math::Vec3 hsv_to_rgb(float h, float s, float v) {
	math::Vec3 hue = hue_to_rgb(h);
	return (((hue - 1.0f) * s + 1.0f) * v).saturated();
}

// http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
math::Vec3 k_to_rbg(float k) {
	double k_100 = std::min(40000.0, std::max(1000.0, double(k))) / 100.0;
	double r = k_100 <= 66.0
		? 255.0
		: 329.698727446 * std::pow(k_100 - 60.0, -0.1332047592);
	double g = k_100 <= 66.0
		? 99.4708025861 * std::log(k_100) - 161.1195681661
		: 288.1221695283 * std::pow(k_100 - 60.0, -0.0755148492);

	double b = k_100 >= 66.0
		? 255.0
		: k_100 <= 19.0
			? 0.0
			: 138.5177312231 * std::log(k_100 - 10) - 305.0447927307;

	return (math::Vec3(float(r), float(g), float(b)) / 255.0f).saturated();
}

float rgb_to_k(const math::Vec3& rgb) {
	if(rgb.x() >= 1.0f) {
		double g = double(rgb.y()) * 255.0;
		double k_100 = 5.0519153526 * std::exp(0.0100532012812 * g);
		return float(k_100 * 100.0);
	}

	double r = double(rgb.x()) * 255.0;
	double x = std::pow(r, 7.507239275877164);
	double k_100 = (8.018790685011271e18 + 60.0 * x) / x;
	return float(k_100 * 100.0);
}

}

#endif // YAVE_UTILS_COLOR_H
