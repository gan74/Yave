/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_FRAMEGRAPH_TRANSIENTMIPVIEW_H
#define YAVE_FRAMEGRAPH_TRANSIENTMIPVIEW_H

#include <yave/graphics/images/Image.h>
#include <yave/graphics/images/ImageView.h>

#include <yave/graphics/graphics.h>

namespace yave {

class TransientMipViewContainer : NonCopyable {
    public:
        TransientMipViewContainer() = default;

        TransientMipViewContainer(const ImageBase& base, u32 mip);

    protected:
        VkHandle<VkImageView> _view_handle;
};


template<ImageUsage Usage, ImageType Type = ImageType::TwoD>
class TransientMipView : TransientMipViewContainer, public ImageView<Usage, Type>, NonMovable {

    using ImageView<Usage, Type>::is_compatible;

    public:
        TransientMipView() = default;

        template<ImageUsage U, ImageType T> requires(is_compatible(U) && is_compatible(T))
        TransientMipView(const Image<U, T>& img, u32 mip) :
                TransientMipViewContainer(img, mip),
                ImageView<Usage, Type>(
                    img.image_size().to<2>() / (1 << mip),
                    img.usage(),
                    img.format(),
                    _view_handle,
                    img.vk_image()) {
        }

        ~TransientMipView() {
            // Unlike "normal" image views we own the vulkan object
            y_debug_assert(_view_handle == this->vk_view());
            destroy_graphic_resource(std::move(_view_handle));
        }
};


}

#endif // YAVE_FRAMEGRAPH_TRANSIENTMIPVIEW_H
