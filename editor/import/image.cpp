/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "import.h"
#include "transforms.h"

#include <yave/utils/FileSystemModel.h>

#include "stb.h"

namespace editor {
namespace import {

Named<ImageData> import_image(const core::String& filename, ImageImportFlags flags) {
	y_profile();

	int width, height, bpp;
	u8* data = stbi_load(filename.data(), &width, &height, &bpp, 4);
	y_defer(stbi_image_free(data););

	if(!data) {
		y_throw(fmt_c_str("Unable to load image \"%\".", filename));
	}

	ImageData img(math::Vec2ui(width, height), data, vk::Format::eR8G8B8A8Unorm);
	if((flags & ImageImportFlags::GenerateMipmaps) == ImageImportFlags::GenerateMipmaps) {
		img = compute_mipmaps(img);
	}
	return {clean_asset_name(filename), std::move(img)};
}

core::String supported_image_extensions() {
	return "*.jpg;*.jpeg;*.png;*.bmp;*.psd;*.tga;*.gif;*.hdr;*.pic;*.ppm;*.pgm";
}

}
}
