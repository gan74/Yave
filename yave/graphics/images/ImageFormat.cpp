/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include "ImageFormat.h"

namespace yave {

ImageFormat::ImageFormat(vk::Format format) : _format(format) {
}

bool ImageFormat::operator==(const ImageFormat& other) const {
	return _format == other._format;
}

bool ImageFormat::is_valid() const {
	return _format != vk::Format::eUndefined && _format < decltype(_format)(VK_FORMAT_END_RANGE);
}

vk::Format ImageFormat::vk_format() const {
	return _format;
}

usize ImageFormat::bit_per_pixel() const {
	switch(_format) {
		case vk::Format::eBc1RgbUnormBlock:
		case vk::Format::eBc1RgbSrgbBlock:
		case vk::Format::eBc1RgbaUnormBlock:
		case vk::Format::eBc1RgbaSrgbBlock:
			return 4;

		case vk::Format::eR4G4UnormPack8:
		case vk::Format::eR8Unorm:
		case vk::Format::eR8Snorm:
		case vk::Format::eR8Uscaled:
		case vk::Format::eR8Sscaled:
		case vk::Format::eR8Uint:
		case vk::Format::eR8Sint:
		case vk::Format::eR8Srgb:
		case vk::Format::eS8Uint:
		case vk::Format::eBc2UnormBlock:
		case vk::Format::eBc2SrgbBlock:
		case vk::Format::eBc3UnormBlock:
		case vk::Format::eBc3SrgbBlock:
		case vk::Format::eBc4UnormBlock:
		case vk::Format::eBc4SnormBlock:
		case vk::Format::eBc5UnormBlock:
		case vk::Format::eBc5SnormBlock:
		case vk::Format::eBc6HUfloatBlock:
		case vk::Format::eBc6HSfloatBlock:
		case vk::Format::eBc7UnormBlock:
		case vk::Format::eBc7SrgbBlock:
			return 8;

		case vk::Format::eR4G4B4A4UnormPack16:
		case vk::Format::eB4G4R4A4UnormPack16:
		case vk::Format::eR5G6B5UnormPack16:
		case vk::Format::eB5G6R5UnormPack16:
		case vk::Format::eR5G5B5A1UnormPack16:
		case vk::Format::eB5G5R5A1UnormPack16:
		case vk::Format::eA1R5G5B5UnormPack16:
		case vk::Format::eR8G8Unorm:
		case vk::Format::eR8G8Snorm:
		case vk::Format::eR8G8Uscaled:
		case vk::Format::eR8G8Sscaled:
		case vk::Format::eR8G8Uint:
		case vk::Format::eR8G8Sint:
		case vk::Format::eR8G8Srgb:
		case vk::Format::eR16Unorm:
		case vk::Format::eR16Snorm:
		case vk::Format::eR16Uscaled:
		case vk::Format::eR16Sscaled:
		case vk::Format::eR16Uint:
		case vk::Format::eR16Sint:
		case vk::Format::eR16Sfloat:
		case vk::Format::eD16Unorm:
			return 16;

		case vk::Format::eR8G8B8Unorm:
		case vk::Format::eR8G8B8Snorm:
		case vk::Format::eR8G8B8Uscaled:
		case vk::Format::eR8G8B8Sscaled:
		case vk::Format::eR8G8B8Uint:
		case vk::Format::eR8G8B8Sint:
		case vk::Format::eR8G8B8Srgb:
		case vk::Format::eB8G8R8Unorm:
		case vk::Format::eB8G8R8Snorm:
		case vk::Format::eB8G8R8Uscaled:
		case vk::Format::eB8G8R8Sscaled:
		case vk::Format::eB8G8R8Uint:
		case vk::Format::eB8G8R8Sint:
		case vk::Format::eB8G8R8Srgb:
		case vk::Format::eD16UnormS8Uint:
			return 24;

		case vk::Format::eR8G8B8A8Unorm:
		case vk::Format::eR8G8B8A8Snorm:
		case vk::Format::eR8G8B8A8Uscaled:
		case vk::Format::eR8G8B8A8Sscaled:
		case vk::Format::eR8G8B8A8Uint:
		case vk::Format::eR8G8B8A8Sint:
		case vk::Format::eR8G8B8A8Srgb:
		case vk::Format::eB8G8R8A8Unorm:
		case vk::Format::eB8G8R8A8Snorm:
		case vk::Format::eB8G8R8A8Uscaled:
		case vk::Format::eB8G8R8A8Sscaled:
		case vk::Format::eB8G8R8A8Uint:
		case vk::Format::eB8G8R8A8Sint:
		case vk::Format::eB8G8R8A8Srgb:
		case vk::Format::eA8B8G8R8UnormPack32:
		case vk::Format::eA8B8G8R8SnormPack32:
		case vk::Format::eA8B8G8R8UscaledPack32:
		case vk::Format::eA8B8G8R8SscaledPack32:
		case vk::Format::eA8B8G8R8UintPack32:
		case vk::Format::eA8B8G8R8SintPack32:
		case vk::Format::eA8B8G8R8SrgbPack32:
		case vk::Format::eA2R10G10B10UnormPack32:
		case vk::Format::eA2R10G10B10SnormPack32:
		case vk::Format::eA2R10G10B10UscaledPack32:
		case vk::Format::eA2R10G10B10SscaledPack32:
		case vk::Format::eA2R10G10B10UintPack32:
		case vk::Format::eA2R10G10B10SintPack32:
		case vk::Format::eA2B10G10R10UnormPack32:
		case vk::Format::eA2B10G10R10SnormPack32:
		case vk::Format::eA2B10G10R10UscaledPack32:
		case vk::Format::eA2B10G10R10SscaledPack32:
		case vk::Format::eA2B10G10R10UintPack32:
		case vk::Format::eA2B10G10R10SintPack32:
		case vk::Format::eR16G16Unorm:
		case vk::Format::eR16G16Snorm:
		case vk::Format::eR16G16Uscaled:
		case vk::Format::eR16G16Sscaled:
		case vk::Format::eR16G16Uint:
		case vk::Format::eR16G16Sint:
		case vk::Format::eR16G16Sfloat:
		case vk::Format::eR32Uint:
		case vk::Format::eR32Sint:
		case vk::Format::eR32Sfloat:
		case vk::Format::eB10G11R11UfloatPack32:
		case vk::Format::eE5B9G9R9UfloatPack32:
		case vk::Format::eX8D24UnormPack32:
		case vk::Format::eD32Sfloat:
		case vk::Format::eD24UnormS8Uint:
			return 32;

		case vk::Format::eR16G16B16Unorm:
		case vk::Format::eR16G16B16Snorm:
		case vk::Format::eR16G16B16Uscaled:
		case vk::Format::eR16G16B16Sscaled:
		case vk::Format::eR16G16B16Uint:
		case vk::Format::eR16G16B16Sint:
		case vk::Format::eR16G16B16Sfloat:
			return 48;

		case vk::Format::eR16G16B16A16Unorm:
		case vk::Format::eR16G16B16A16Snorm:
		case vk::Format::eR16G16B16A16Uscaled:
		case vk::Format::eR16G16B16A16Sscaled:
		case vk::Format::eR16G16B16A16Uint:
		case vk::Format::eR16G16B16A16Sint:
		case vk::Format::eR16G16B16A16Sfloat:
		case vk::Format::eR32G32Uint:
		case vk::Format::eR32G32Sint:
		case vk::Format::eR32G32Sfloat:
		case vk::Format::eR64Uint:
		case vk::Format::eR64Sint:
		case vk::Format::eR64Sfloat:
			return 64;

		case vk::Format::eR32G32B32Uint:
		case vk::Format::eR32G32B32Sint:
		case vk::Format::eR32G32B32Sfloat:
			return 96;

		case vk::Format::eR32G32B32A32Uint:
		case vk::Format::eR32G32B32A32Sint:
		case vk::Format::eR32G32B32A32Sfloat:
		case vk::Format::eR64G64Uint:
		case vk::Format::eR64G64Sint:
		case vk::Format::eR64G64Sfloat:
			return 128;

		case vk::Format::eR64G64B64Uint:
		case vk::Format::eR64G64B64Sint:
		case vk::Format::eR64G64B64Sfloat:
			return 192;

		case vk::Format::eR64G64B64A64Uint:
		case vk::Format::eR64G64B64A64Sint:
		case vk::Format::eR64G64B64A64Sfloat:
			return 256;

		default:
			return y_fatal("Unsupported image format.");
	}
}

vk::ImageAspectFlags ImageFormat::vk_aspect() const {
	switch(_format) {
		case vk::Format::eD16Unorm:
		case vk::Format::eD32Sfloat:
			return vk::ImageAspectFlagBits::eDepth;

		case vk::Format::eD16UnormS8Uint:
		case vk::Format::eD24UnormS8Uint:
		case vk::Format::eD32SfloatS8Uint:
			return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

		default:
			return vk::ImageAspectFlagBits::eColor;
	}
}


}
