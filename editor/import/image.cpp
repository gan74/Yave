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

#include "image.h"

#include <yave/utils/FileSystemModel.h>

extern "C" {
#define STB_IMAGE_IMPLEMENTATION
#include <external/stb/stb_image.h>
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

core::Vector<Named<ImageData>> import_images(const core::String& path) {
	int width, height, bpp;
	// stbi_set_flip_vertically_on_load(true);
	u8* rgb = stbi_load(path.data(), &width, &height, &bpp, 4);
	y_defer(stbi_image_free(rgb););

	if(!rgb) {
		y_throw("Unable to load image.");
	}

	if(bpp != 3 && bpp != 4) {
		y_throw("Unable to load image.");
	}

	core::Vector<Named<ImageData>> images;
	images.emplace_back(name_from_path(path), ImageData(math::Vec2ui(width, height), rgb, ImageFormat(bpp == 3 ? vk::Format::eR8G8B8Unorm : vk::Format::eR8G8B8A8Unorm)));
	return images;
}

core::String supported_image_extensions() {
	return "*.jpg;*.jpeg;*.png;*.bmp;*.psd;*.tga;*.gif;*.hdr;*.pic;*.ppm;*.pgm";
}

}
}
