/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_IMAGES_IMAGEVIEW_H
#define YAVE_GRAPHICS_IMAGES_IMAGEVIEW_H

#include <yave/yave.h>

#include "Image.h"

namespace yave {

template<ImageUsage Usage, ImageType Type = ImageType::TwoD>
class ImageView {

    protected:
        static constexpr bool is_compatible(ImageUsage u) {
            return (uenum(Usage) & uenum(u)) == uenum(Usage);
        }

        static constexpr bool is_compatible(ImageType t) {
            return Type == t || (Type == ImageType::Layered && t == ImageType::Cube);
        }

    public:
        using size_type = typename Image<Usage, Type>::size_type;

        ImageView() = default;

        template<ImageUsage U, ImageType T> requires(is_compatible(U) && is_compatible(T))
        ImageView(const Image<U, T>& img) : ImageView(img.size(), img.usage(), img.format(), img.vk_view(), img.vk_image()) {
            static_assert(is_compatible(U));
            static_assert(is_compatible(T));
        }

        template<ImageUsage U, ImageType T> requires(is_compatible(U) && is_compatible(T))
        ImageView(const ImageView<U, T>& img) : ImageView(img.size(), img.usage(), img.format(), img.vk_view(), img.vk_image()) {
            static_assert(is_compatible(U));
            static_assert(is_compatible(T));
        }

        bool is_null() const {
            return !_view;
        }

        VkImageView vk_view() const {
            return _view;
        }

        VkImage vk_image() const {
            return _image;
        }

        ImageUsage usage() const {
            return _usage;
        }

        ImageFormat format() const {
            return _format;
        }

        const size_type& size() const {
            return _size;
        }

        bool operator==(const ImageView& other) const {
            return _view == other._view;
        }

        bool operator!=(const ImageView& other) const {
            return !operator==(other);
        }

        VkDescriptorImageInfo vk_descriptor_info(SamplerType sampler) const {
            VkDescriptorImageInfo info = {};
            {
                info.sampler = vk_sampler(sampler);
                info.imageView = _view;
                info.imageLayout = vk_image_layout(_usage);
            }
            return info;
        }

    protected:
        ImageView(const size_type& size, ImageUsage usage, ImageFormat format, VkImageView view, VkImage image) :
                _size(size),
                _usage(usage),
                _format(format),
                _view(view),
                _image(image) {
            y_debug_assert(_view && _image);
        }


    private:
        template<ImageUsage U, ImageType T>
        friend class ImageView;

        size_type _size;
        ImageUsage _usage = ImageUsage::None;
        ImageFormat _format;
        VkImageView _view = {};
        VkImage _image = {};
};

using TextureView = ImageView<ImageUsage::TextureBit>;
using StorageView = ImageView<ImageUsage::StorageBit>;
using DepthAttachmentView = ImageView<ImageUsage::DepthBit>;
using ColorAttachmentView = ImageView<ImageUsage::ColorBit>;
using DepthTextureAttachmentView = ImageView<ImageUsage::DepthBit | ImageUsage::TextureBit>;
using ColorTextureAttachmentView = ImageView<ImageUsage::ColorBit | ImageUsage::TextureBit>;

using CubemapView = ImageView<ImageUsage::TextureBit, ImageType::Cube>;
using CubemapStorageView = ImageView<ImageUsage::StorageBit, ImageType::Cube>;


}

#endif // YAVE_GRAPHICS_IMAGES_IMAGEVIEW_H

