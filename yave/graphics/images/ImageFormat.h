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
#ifndef YAVE_GRAPHICS_IMAGES_IMAGEFORMAT_H
#define YAVE_GRAPHICS_IMAGES_IMAGEFORMAT_H

#include <yave/graphics/vk/vk.h>

#include <y/reflect/reflect.h>

namespace yave {

class ImageFormat {
    public:
        constexpr ImageFormat(VkFormat format = VK_FORMAT_UNDEFINED) : _format(format) {
        }

        VkFormat vk_format() const;
        VkImageAspectFlags vk_aspect() const;

        usize bit_per_pixel() const;
        usize components() const;

        bool is_valid() const;
        bool is_float() const;

        bool supports_filtering() const;

        bool is_block_format() const;
        bool is_depth_format() const;

        ImageFormat non_depth() const;

        std::string_view name() const;

        bool operator==(const ImageFormat& other) const;
        bool operator!=(const ImageFormat& other) const;

        y_reflect(_format)

    private:
        VkFormat _format;
};

}

#endif // YAVE_GRAPHICS_IMAGES_IMAGEFORMAT_H

