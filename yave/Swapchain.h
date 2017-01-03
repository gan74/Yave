/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef YAVE_SWAPCHAIN_H
#define YAVE_SWAPCHAIN_H

#include "yave.h"

#include "Framebuffer.h"

#include <yave/image/ImageFormat.h>
#include <yave/image/Image.h>
#include <yave/DeviceLinked.h>

namespace yave {

class Window;

static constexpr ImageUsage SwapchainImageUsage = ImageUsage::Swapchain | ImageUsage::Color | ImageUsage::Storage;

class SwapchainImage : public Image<SwapchainImageUsage> {
	private:
		friend class Swapchain;

		struct DeviceSetter : DeviceLinked {
			DeviceSetter(DevicePtr dptr) : DeviceLinked(dptr) {
			}
		};

		SwapchainImage(DevicePtr dptr) : Image<SwapchainImageUsage>() {
			DeviceSetter dev(dptr);
			DeviceLinked::swap(dev);
		}
};

using SwapchainImageView = ImageView<SwapchainImageUsage>;

class Swapchain : NonCopyable, public DeviceLinked {

	public:
#ifdef Y_OS_WIN
		Swapchain(DevicePtr dptr, HINSTANCE instance, HWND handle);
#endif

		Swapchain(DevicePtr dptr, vk::SurfaceKHR&& surface);
		Swapchain(DevicePtr dptr, Window* window);
		~Swapchain();

		const Framebuffer& framebuffer(usize index) const;
		const RenderPass& render_pass() const;

		vk::SwapchainKHR vk_swapchain() const;

		const math::Vec2ui& size() const;
		usize image_count() const;

		ImageFormat color_format() const;

		const auto& images() const {
			return _images;
		}

		auto images() {
			return core::range(_images);
		}


	private:
		math::Vec2ui _size;
		ImageFormat _color_format;

		core::Vector<SwapchainImage> _images;

		Owner<vk::SurfaceKHR> _surface;
		vk::SwapchainKHR _swapchain;

};





}

#endif // YAVE_SWAPCHAIN_H
