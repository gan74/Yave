/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_FRAMEGRAPH_TRANSIENTIMAGE_H
#define YAVE_FRAMEGRAPH_TRANSIENTIMAGE_H

#include <yave/graphics/images/Image.h>
#include <yave/graphics/images/ImageView.h>

namespace yave {

template<ImageType Type = ImageType::TwoD>
class TransientImage final : public ImageBase {
    static constexpr bool is_3d = Type == ImageType::ThreeD;

    template<typename T>
    math::Vec3ui to_3d_size(const T& size) {
        math::Vec3ui s(1);
        s.to<T::size()>() = size;
        return s;
    }

    public:
        using size_type = std::conditional_t<is_3d, math::Vec3ui, math::Vec2ui>;

        TransientImage() = default;

        TransientImage(ImageFormat format, ImageUsage usage, const size_type& image_size) : ImageBase(format, usage, to_3d_size(image_size)) {
        }

        TransientImage(TransientImage&&) = default;
        TransientImage& operator=(TransientImage&&) = default;

        template<ImageUsage U>
        TransientImage(Image<U, Type>&& other) {
            ImageBase::operator=(other);
        }

        template<ImageUsage U>
        TransientImage& operator=(Image<U, Type>&& other) {
            ImageBase::operator=(other);
            return *this;
        }

        const size_type& size() const {
            return image_size().to<size_type::size()>();
        }
};

template<ImageUsage Usage, ImageType Type = ImageType::TwoD>
class TransientImageView final : public ImageView<Usage, Type> {
    public:
        TransientImageView(const TransientImage<Type>& image) :
                ImageView<Usage, Type>(image.size(), image.usage(), image.format(), image.vk_view(), image.vk_image()) {

            if(!ImageView<Usage, Type>::is_compatible(image.usage())) {
                y_fatal("Invalid image view.");
            }
        }
};

}

#endif // YAVE_FRAMEGRAPH_TRANSIENTIMAGE_H

