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
#ifndef YAVE_GRAPHICS_BUFFERS_BUFFER_H
#define YAVE_GRAPHICS_BUFFERS_BUFFER_H

#include <yave/graphics/memory/DeviceMemory.h>

#include "BufferUsage.h"

#include <yave/utils/traits.h>

namespace yave {

template<BufferUsage Usage, MemoryType Memory>
class SubBuffer;

class BufferBase : NonCopyable {

    public:
        ~BufferBase();

        BufferUsage usage() const;
        usize byte_size() const;
        const DeviceMemory& device_memory() const;

        VkDescriptorBufferInfo descriptor_info() const;

        bool is_null() const;

        VkBuffer vk_buffer() const;

    protected:
        BufferBase() = default;
        BufferBase(BufferBase&&) = default;
        BufferBase& operator=(BufferBase&&) = default;

        BufferBase(usize byte_size, BufferUsage usage, MemoryType type);

    private:
        usize _size = 0;
        BufferUsage _usage = BufferUsage::None;

        VkHandle<VkBuffer> _buffer;
        DeviceMemory _memory;
};

static_assert(is_safe_base<BufferBase>::value);

template<BufferUsage Usage, MemoryType Memory = prefered_memory_type(Usage)>
class Buffer : public BufferBase {

    protected:
        template<typename T>
        static constexpr bool has(T a, T b) {
            return (uenum(a) & uenum(b)) == uenum(b);
        }

        static constexpr bool is_compatible(BufferUsage U, MemoryType M) {
            return has(U, Usage) && is_memory_type_compatible(M, Memory);
        }

    public:
        static constexpr BufferUsage usage = Usage;
        static constexpr MemoryType memory_type = Memory;

        using sub_buffer_type = SubBuffer<usage, memory_type>;
        using base_buffer_type = Buffer<usage, memory_type>;


        static usize total_byte_size(usize size) {
            return size;
        }


        Buffer() = default;


        // This is important: it prevent the ctor from being instanciated for Buffer specialisations that should not be created this way,
        // thus preventing static_assert from going off.
        template<typename = void>
        Buffer(usize byte_size) : BufferBase(byte_size, Usage, Memory) {
            static_assert(Usage != BufferUsage::None, "Buffers should not have Usage == BufferUsage::None");
            static_assert(Memory != MemoryType::DontCare, "Buffers should not have Memory == MemoryType::DontCare");
        }

        template<BufferUsage U, MemoryType M>
        Buffer(Buffer<U, M>&& other) {
            static_assert(is_compatible(U, M));
            BufferBase::operator=(other);
        }

        template<BufferUsage U, MemoryType M>
        Buffer& operator=(Buffer<U, M>&& other) {
            static_assert(is_compatible(U, M));
            BufferBase::operator=(other);
            return *this;
        }
};

}

#endif // YAVE_GRAPHICS_BUFFERS_BUFFER_H

