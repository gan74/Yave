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
#ifndef YAVE_GRAPHICS_SWAPCHAIN_SWAPCHAIN_H
#define YAVE_GRAPHICS_SWAPCHAIN_SWAPCHAIN_H

#include <yave/yave.h>

#include <yave/graphics/images/ImageFormat.h>
#include <yave/graphics/images/Image.h>
#include <yave/graphics/device/DeviceLinked.h>
#include <yave/graphics/framebuffer/Framebuffer.h>
#include <yave/window/Window.h>

#include "FrameToken.h"

namespace yave {

static constexpr ImageUsage SwapchainImageUsage = ImageUsage::SwapchainBit | ImageUsage::ColorBit;
using SwapchainImageView = ImageView<SwapchainImageUsage>;

class Swapchain : NonMovable, public DeviceLinked {

    class SwapchainImage : public Image<SwapchainImageUsage> {
        public:
            SwapchainImage(SwapchainImage&&) = default;
            SwapchainImage& operator=(SwapchainImage&&) = default;

            ~SwapchainImage() {
                // prevents images to delete their VkImage, this is already done by the swapchain
                _image = {};
            }

        private:
            friend class Swapchain;

            SwapchainImage() = default;
    };

    public:
#ifdef Y_OS_WIN
        Swapchain(DevicePtr dptr, HINSTANCE_ instance, HWND_ handle);
#endif

        Swapchain(DevicePtr dptr, VkSurfaceKHR surface);
        Swapchain(DevicePtr dptr, Window* window);
        ~Swapchain();

        void reset();

        VkSwapchainKHR vk_swapchain() const;

        const math::Vec2ui& size() const;
        usize image_count() const;

        ImageFormat color_format() const;

        const auto& images() const {
            return _images;
        }

        bool is_valid() const;

        FrameToken next_frame();
        void present(const FrameToken& token, VkQueue queue);

    private:
        void build_swapchain();
        void build_semaphores();
        void destroy_semaphores();


        u64 _frame_id = 0;

        math::Vec2ui _size;
        ImageFormat _color_format;

        core::Vector<SwapchainImage> _images;
        core::Vector<std::pair<VkSemaphore, VkSemaphore>> _semaphores;

        VkHandle<VkSurfaceKHR> _surface;
        VkHandle<VkSwapchainKHR> _swapchain;


};





}

#endif // YAVE_GRAPHICS_SWAPCHAIN_SWAPCHAIN_H

