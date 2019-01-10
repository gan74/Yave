/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_FRAMEBUFFER_RENDERPASS_H
#define YAVE_GRAPHICS_FRAMEBUFFER_RENDERPASS_H

#include <yave/yave.h>

#include <yave/device/DeviceLinked.h>
#include <yave/device/DeviceLinked.h>
#include <yave/graphics/images/Image.h>
#include <yave/graphics/images/ImageView.h>

namespace yave {



class RenderPass : NonCopyable, public DeviceLinked {
	public:
		struct ImageData {
			const ImageFormat format = vk::Format::eUndefined;
			const ImageUsage usage = ImageUsage::None;

			ImageData() = default;

			ImageData(ImageFormat fmt, ImageUsage us) : format(fmt), usage(us) {
			}

			ImageData(const ImageBase& img) : format(img.format()), usage(img.usage()) {
			}

			template<ImageUsage Usage>
			ImageData(const ImageView<Usage>& img) : format(img.format()), usage(img.usage()) {
			}
		};

		class Layout {
			public:
				Layout() = default;
				Layout(ImageData depth, core::ArrayView<ImageData> colors);

				u64 hash() const;
				bool is_depth_only() const;

				bool operator==(const Layout& other) const;

			private:
				ImageFormat _depth;
				core::SmallVector<ImageFormat, 7> _colors;
		};

		RenderPass() = default;
		RenderPass(DevicePtr dptr, ImageData depth, core::ArrayView<ImageData> colors);
		RenderPass(DevicePtr dptr, core::ArrayView<ImageData> colors);

		RenderPass(RenderPass&& other);
		RenderPass& operator=(RenderPass&& other);

		~RenderPass();

		bool is_depth_only() const;

		const Layout& layout() const;
		usize attachment_count() const;

		vk::RenderPass vk_render_pass() const;

	private:
		void swap(RenderPass& other);

		usize _attachment_count = 0;
		vk::RenderPass _render_pass;

		Layout _layout;
};

}

#endif // YAVE_GRAPHICS_FRAMEBUFFER_RENDERPASS_H
