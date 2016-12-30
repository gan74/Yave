/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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

class SwapchainImage : public Image<ImageUsage::Swapchain | ImageUsage::Color> {
	private:
		friend class Swapchain;

		SwapchainImage() : Image<ImageUsage::Swapchain | ImageUsage::Color>() {
		}
};

using SwapchainImageView = ImageView<ImageUsage::Swapchain | ImageUsage::Color>;

class Swapchain : NonCopyable, public DeviceLinked {

	struct Buffer {
		SwapchainImage color;
		DepthAttachment depth;
		Framebuffer framebuffer;

		Buffer(RenderPass& render_pass, SwapchainImage&& color_att, DepthAttachment&& depth_att);
	};

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
		usize buffer_count() const;

	private:
		math::Vec2ui _size;
		ImageFormat _depth_format;
		ImageFormat _color_format;

		core::Vector<Buffer> _buffers;
		RenderPass _render_pass;

		Owner<vk::SurfaceKHR> _surface;
		vk::SwapchainKHR _swapchain;

};





}

#endif // YAVE_SWAPCHAIN_H
