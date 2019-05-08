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

}

#endif // YAVE_UTILS_COLOR_H
