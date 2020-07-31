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
#ifndef YAVE_GRAPHICS_BUFFERS_SUBBUFFERBASE_H
#define YAVE_GRAPHICS_BUFFERS_SUBBUFFERBASE_H

#include <yave/graphics/memory/DeviceMemoryView.h>

#include "Buffer.h"

namespace yave {

class SubBufferBase {

    public:
        SubBufferBase() = default;
        SubBufferBase(const BufferBase& base, usize byte_len, usize byte_off);

        explicit SubBufferBase(const BufferBase& base);

        DevicePtr device() const;
        bool is_null() const;

        usize byte_size() const;
        usize byte_offset() const;

        VkBuffer vk_buffer() const;

        DeviceMemoryView device_memory() const;

        VkDescriptorBufferInfo descriptor_info() const;
        VkMappedMemoryRange vk_memory_range() const;

    protected:
        static usize alignment_for_usage(DevicePtr dptr, BufferUsage usage);

    private:
        usize _size = 0;
        usize _offset = 0;
        NotOwner<VkBuffer> _buffer = {};
        DeviceMemoryView _memory;
};

// in this case it's ok
//static_assert(is_safe_base<SubBufferBase>::value);

}

#endif // YAVE_GRAPHICS_BUFFERS_SUBBUFFERBASE_H

