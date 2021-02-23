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
#ifndef YAVE_GRAPHICS_SWAPCHAIN_FRAMETOKEN_H
#define YAVE_GRAPHICS_SWAPCHAIN_FRAMETOKEN_H

#include <yave/graphics/images/ImageView.h>

namespace yave {

struct FrameToken {
    static constexpr u64 invalid_id = u64(-1);

    const u64 id;
    const u32 image_index;
    const u32 image_count;

    const ImageView<ImageUsage::ColorBit> image_view;

    bool operator==(const FrameToken& other) const {
        return id == other.id &&
               image_view == other.image_view &&
               image_index == other.image_index &&
               image_count == other.image_count;
    }

    bool operator!=(const FrameToken& other) const {
        return !operator==(other);
    }

    static FrameToken create_disposable(ImageView<ImageUsage::ColorBit> out_view) {
        return FrameToken {
                invalid_id,
                0,
                1,
                out_view
            };
    }

};



}

#endif // YAVE_GRAPHICS_SWAPCHAIN_FRAMETOKEN_H

