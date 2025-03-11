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
#ifndef YAVE_GRAPHICS_BUFFERS_BUFFER_H
#define YAVE_GRAPHICS_BUFFERS_BUFFER_H

#include "BufferBase.h"
#include "BufferMapping.h"

#include <yave/utils/traits.h>

namespace yave {

template<BufferUsage Usage, MemoryType Memory>
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


        static u64 total_byte_size(u64 size) {
            return size;
        }


        Buffer() = default;


        // This is important: it prevent the ctor from being instanciated for Buffer specialisations that should not be created this way,
        // thus preventing static_assert from going off.
        template<typename = void>
        explicit Buffer(u64 byte_size, MemoryAllocFlags alloc_flags = MemoryAllocFlags::None) : BufferBase(byte_size, Usage, Memory, alloc_flags) {
            static_assert(Usage != BufferUsage::None, "Buffers should not have Usage == BufferUsage::None");
            static_assert(Memory != MemoryType::DontCare, "Buffers should not have Memory == MemoryType::DontCare");
        }

        template<BufferUsage U, MemoryType M>
        explicit Buffer(Buffer<U, M>&& other) {
            static_assert(is_compatible(U, M));
            BufferBase::operator=(other);
        }

        template<BufferUsage U, MemoryType M>
        Buffer& operator=(Buffer<U, M>&& other) {
            static_assert(is_compatible(U, M));
            BufferBase::operator=(other);
            return *this;
        }


        template<typename = void>
        BufferMapping<u8> map_bytes(MappingAccess access) const {
            static_assert(is_cpu_visible(memory_type));
            return BufferMapping<u8>(*this, access);
        }
};

template<BufferUsage Usage, MemoryType Memory>
class SubBuffer : public SubBufferBase {
    protected:
        template<typename T>
        static constexpr bool has(T a, T b) {
            return (uenum(a) & uenum(b)) == uenum(b);
        }

        static constexpr bool is_compatible(BufferUsage U, MemoryType M) {
            return has(U, Usage) && is_memory_type_compatible(M, Memory);
        }

        static_assert(!has(BufferUsage::UniformBit, BufferUsage::UniformBit | BufferUsage::IndirectBit));
        static_assert(has(BufferUsage::UniformBit | BufferUsage::IndirectBit, BufferUsage::UniformBit));

        explicit SubBuffer(const BufferBase& buffer) : SubBufferBase(buffer) {
        }

        explicit SubBuffer(const SubBufferBase& buffer) : SubBufferBase(buffer) {
        }


    public:
        static constexpr BufferUsage usage = Usage;
        static constexpr MemoryType memory_type = Memory;

        using sub_buffer_type = SubBuffer<Usage, memory_type>;
        using base_buffer_type = Buffer<Usage, memory_type>;


        static u64 byte_alignment() {
            return buffer_alignment_for_usage(Usage);
        }


        SubBuffer() = default;

        template<BufferUsage U, MemoryType M>
        SubBuffer(const SubBuffer<U, M>& buffer) : SubBufferBase(buffer) {
            static_assert(is_compatible(U, M));
        }

        template<BufferUsage U, MemoryType M>
        SubBuffer(const Buffer<U, M>& buffer) : SubBufferBase(buffer, buffer.byte_size(), 0) {
            static_assert(is_compatible(U, M));
        }

        // these are dangerous with typedwrapper as it's never clear what is in byte and what isn't.
        // todo find some way to make this better

        template<BufferUsage U, MemoryType M>
        SubBuffer(const SubBuffer<U, M>& buffer, u64 byte_len, u64 byte_off) : SubBufferBase(buffer, byte_len, byte_off) {
            static_assert(is_compatible(U, M));
            y_debug_assert(buffer.byte_size() >= byte_len + byte_off);
            y_debug_assert(byte_offset() % byte_alignment() == 0);
            if constexpr(M == MemoryType::CpuVisible) {
                y_debug_assert(byte_offset() % host_side_alignment() == 0);
            }
        }

        template<BufferUsage U, MemoryType M>
        SubBuffer(const Buffer<U, M>& buffer, u64 byte_len, u64 byte_off) : SubBuffer(SubBuffer<U, M>(buffer), byte_len, byte_off) {
        }


        template<typename = void>
        BufferMapping<u8> map_bytes(MappingAccess access) const {
            static_assert(is_cpu_visible(memory_type));
            return BufferMapping<u8>(*this, access);
        }
};


template<BufferUsage U, MemoryType M>
SubBuffer(const Buffer<U, M>&) -> SubBuffer<U, M>;






template<typename Elem, typename Buff>
class TypedWrapper final : public Buff {

    static constexpr bool is_sub = std::is_base_of_v<SubBufferBase, Buff>;
    static constexpr bool is_buf = std::is_base_of_v<BufferBase, Buff>;
    static_assert(is_buf || is_sub);

    public:
        using Buff::usage;
        using Buff::memory_type;

        using sub_buffer_type = TypedWrapper<Elem, typename Buff::sub_buffer_type>;
        using base_buffer_type = Buffer<usage, memory_type>;

        using value_type = Elem;

        static_assert(
            !(usage == BufferUsage::StorageBit || usage == BufferUsage::UniformBit) ||
            (sizeof(Elem) == sizeof(u32)) ||
            (sizeof(Elem) == sizeof(u32) * 2) ||
            sizeof(Elem) % (sizeof(u32) * 4) == 0,
            "Element size should be a multiple of vec4's (or float or vec2)"
        );

        static u64 total_byte_size(u64 size) {
            return size * sizeof(value_type);
        }


        TypedWrapper() = default;

        explicit TypedWrapper(usize elem_count) : Buff(std::max(usize(1), elem_count) * sizeof(value_type)) {
        }


        template<BufferUsage U, MemoryType M>
        TypedWrapper(const Buffer<U, M>& buffer) : Buff(buffer) {
        }

        template<BufferUsage U, MemoryType M>
        TypedWrapper(const SubBuffer<U, M>& buffer) : Buff(buffer) {
            static_assert(is_sub);
        }

        template<BufferUsage U, MemoryType M>
        TypedWrapper(const Buffer<U, M>& buffer, usize size, usize byte_offset) : Buff(buffer, size * sizeof(value_type), byte_offset) {
            static_assert(is_sub);
        }

        template<typename = void>
        TypedWrapper(const TypedWrapper& buffer, usize size, usize offset) : Buff(buffer, size * sizeof(value_type), offset * sizeof(value_type)) {
            static_assert(is_sub);
        }



        u64 size() const {
            return this->byte_size() / sizeof(value_type);
        }

        u64 element_size() const {
            return sizeof(value_type);
        }

        u64 offset() const {
            if constexpr(is_sub) {
                return this->byte_offset() / sizeof(value_type);
            } else {
                return 0;
            }
        }

        template<typename = void>
        BufferMapping<Elem> map(MappingAccess access) const {
            static_assert(is_cpu_visible(memory_type));
            return BufferMapping<Elem>(*this, access);
        }
};

}

#endif // YAVE_GRAPHICS_BUFFERS_BUFFER_H

