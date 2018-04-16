/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "images.h"

#include <y/core/Chrono.h>

namespace editor {
namespace images {

static u32 to_rgba(char c) {
	switch(c) {
		case '#':
			return 0xFF000000;
		case '.':
			return 0xFFFFFFFF;

		default:
			break;
	}
	return 0x00;
}

static u32 avg(u32 x, u32 y, u32 z, u32 w) {
	u32 r = ((x & 0xFF) + (y & 0xFF) + (z & 0xFF) + (w & 0xFF)) / 4;
	x >>= 8; y >>= 8; z >>= 8; w >>= 8;
	u32 g = ((x & 0xFF) + (y & 0xFF) + (z & 0xFF) + (w & 0xFF)) / 4;
	x >>= 8; y >>= 8; z >>= 8; w >>= 8;
	u32 b = ((x & 0xFF) + (y & 0xFF) + (z & 0xFF) + (w & 0xFF)) / 4;
	x >>= 8; y >>= 8; z >>= 8; w >>= 8;
	u32 a = ((x & 0xFF) + (y & 0xFF) + (z & 0xFF) + (w & 0xFF)) / 4;
	return (a << 24) | (b << 16) | (g << 8) | r;
}

static u32 compute_mips(u32* data, usize size) {
	usize half = size / 2;
	if(half * 2 != size) {
		return 0;
	}

	u32* mip = data + (size * size);
	for(usize y = 0; y != half; ++y) {
		usize mip_offset = y * half;
		for(usize x = 0; x != half; ++x) {
			usize x2 = 2 * x;
			usize y2 = 2 * y;
			u32 a = avg(data[(x2 + 0) + (y2 + 0) * size],
						data[(x2 + 1) + (y2 + 0) * size],
						data[(x2 + 0) + (y2 + 1) * size],
						data[(x2 + 1) + (y2 + 1) * size]);
			mip[x + mip_offset] = a;
		}
	}
	return compute_mips(mip, half) + 1;
}

static ImageData to_image(const char* ascii, const math::Vec2ui& size) {
	usize total = size.x() * size.y();
	auto rgba = std::make_unique<u32[]>(total + total / 2);

	for(usize y = 0; y != size.y(); ++y) {
		usize offset = y * size.x();
		usize row_offset = (size.y() - y - 1) * size.x();
		for(usize x = 0; x != size.x(); ++x) {
			rgba[x + offset] = to_rgba(ascii[x + row_offset]);
		}
	}

	u32 mips = 1;
	if(size.x() == size.y()) {
		mips += compute_mips(rgba.get(), size.x());
	}

	const u8* data = reinterpret_cast<const u8*>(rgba.get());
	return ImageData(size, data, vk::Format::eR8G8B8A8Unorm, mips);

}

ImageData light() {
	const char* ascii =
		"                             ##########                         "
		"                         ##################                     "
		"                       ######################                   "
		"                     ##########################                 "
		"                    #######..............#######                "
		"                   ######..................######               "
		"                  #####......................#####              "
		"                 #####........................#####             "
		"                ####...........................#####            "
		"                ####............................####            "
		"               ####..............................####           "
		"               ###................................###           "
		"              ####................................####          "
		"              ###..................................###          "
		"             ####..................................####         "
		"             ####..................................####         "
		"             ###....................................###         "
		"             ###....................................###         "
		"            ####....................................####        "
		"            ####....................................####        "
		"            ####....................................####        "
		"            ####....................................####        "
		"            ####....................................####        "
		"            ####....................................####        "
		"            ####....................................####        "
		"            ####....................................####        "
		"            ####....................................###         "
		"             ###....................................###         "
		"             ####..................................####         "
		"             ####..................................####         "
		"              ###..................................###          "
		"              ####................................####          "
		"               ###................................###           "
		"               ####..............................####           "
		"                ####............................####            "
		"                ####............................####            "
		"                 ####..........................####             "
		"                 #####........................#####             "
		"                  ####........................####              "
		"                   ####......................####               "
		"                    ####....................####                "
		"                     ####..................####                 "
		"                      ####................####                  "
		"                      ####................####                  "
		"                       ###................###                   "
		"                       ####..............####                   "
		"                       ####..............####                   "
		"                        ###..............###                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                        ####################                    "
		"                            ############                        "
		"                             ##########                         "
		"                              ########                          "
		"                               ######                           ";

	return to_image(ascii, math::Vec2ui(64));
}

}
}
