/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_BUFFERS_BUFFERBASE_H
#define YAVE_GRAPHICS_BUFFERS_BUFFERBASE_H

#include <yave/graphics/memory/DeviceMemory.h>
#include <yave/graphics/memory/DeviceMemoryView.h>

#include "BufferUsage.h"

namespace yave {

class SubBufferBase {
    public:
        SubBufferBase() = default;
        SubBufferBase(const BufferBase& base, u64 byte_len, u64 byte_off);
        SubBufferBase(const SubBufferBase& base, u64 byte_len, u64 byte_off);

        explicit SubBufferBase(const BufferBase& base);

        bool is_null() const;

        u64 byte_size() const;
        u64 byte_offset() const;

        VkBuffer vk_buffer() const;

        DeviceMemoryView device_memory() const;

        VkDescriptorBufferInfo descriptor_info() const;
        VkMappedMemoryRange vk_memory_range() const;

        bool operator==(const SubBufferBase& other) const;
        bool operator!=(const SubBufferBase& other) const;

        static u64 host_side_alignment();

    protected:
        static u64 alignment_for_usage(BufferUsage usage);

    private:
        u64 _size = 0;
        u64 _offset = 0;
        VkBuffer _buffer = {};
        DeviceMemoryView _memory;
};

class BufferBase : NonCopyable {

    public:
        ~BufferBase();

        BufferUsage usage() const;
        u64 byte_size() const;
        const DeviceMemory& device_memory() const;

        VkDescriptorBufferInfo descriptor_info() const;

        bool is_null() const;

        VkBuffer vk_buffer() const;

    protected:
        BufferBase() = default;
        BufferBase(BufferBase&&) = default;
        BufferBase& operator=(BufferBase&&) = default;

        BufferBase(u64 byte_size, BufferUsage usage, MemoryType type);

    private:
        u64 _size = 0;
        BufferUsage _usage = BufferUsage::None;

        VkHandle<VkBuffer> _buffer;
        DeviceMemory _memory;
};


}

#endif // YAVE_GRAPHICS_BUFFERS_BUFFERBASE_H

