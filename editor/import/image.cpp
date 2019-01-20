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

#include "image.h"

#include <yave/utils/FileSystemModel.h>

extern "C" {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#define STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <external/stb/stb_image.h>
//#include <external/stb/stb_image_write.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

namespace editor {
namespace import {

static std::string_view name_from_path(const core::String& path) {
	auto begin = path.begin();
	for(auto i = path.begin(); i != path.end(); ++i) {
		if(*i == '/' || *i == '\\') {
			begin = i;
		}
	}
	++begin;

	auto end = std::find(begin, path.end(), '.');
	return end == begin ? "unamed_image" : std::string_view(begin, end - begin);
}

/*std::unique_ptr<u8[]> rgb_to_rgba(int w, int h, u8* rgb) {
	auto rgba = std::make_unique<u8[]>(w * h * 4);

	for(usize i = 0; i != usize(w * h); ++i) {
		rgba[i * 4 + 0] = rgb[i * 3 + 0];
		rgba[i * 4 + 1] = rgb[i * 3 + 1];
		rgba[i * 4 + 2] = rgb[i * 3 + 2];
		rgba[i * 4 + 3] = 0xFF;
	}

	return rgba;
}*/

core::Vector<Named<ImageData>> import_images(const core::String& path) {
	int width, height, bpp;
	// stbi_set_flip_vertically_on_load(true);
	u8* raw = stbi_load(path.data(), &width, &height, &bpp, 4);
	y_defer(stbi_image_free(raw););

	if(!raw) {
		y_throw("Unable to load image.");
	}

	u8* data = raw;
	/*std::unique_ptr<u8[]> rgba;
	if(bpp == 3) {
		rgba = rgb_to_rgba(width, height, raw);
		data = rgba.get();
	} else if (bpp != 4) {
		y_throw("Unable to load image.");
	}*/

	core::Vector<Named<ImageData>> images;
	images.emplace_back(name_from_path(path), ImageData(math::Vec2ui(width, height), data, vk::Format::eR8G8B8A8Unorm));
	return images;
}

core::String supported_image_extensions() {
	return "*.jpg;*.jpeg;*.png;*.bmp;*.psd;*.tga;*.gif;*.hdr;*.pic;*.ppm;*.pgm";
}

}
}
