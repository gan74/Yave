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
#ifndef YAVE_GRAPHICS_BUFFERS_BUFFERMAPPING_H
#define YAVE_GRAPHICS_BUFFERS_BUFFERMAPPING_H

#include "buffers.h"
#include "BufferBase.h"

namespace yave {

class BufferMappingBase : NonMovable {

    public:
        BufferMappingBase() = default;
        ~BufferMappingBase();

        usize byte_size() const;

        void* raw_data();
        const void* raw_data() const;

    protected:
        void swap(BufferMappingBase& other);

        BufferMappingBase(const SubBuffer<BufferUsage::None, MemoryType::CpuVisible>& buffer, MappingAccess access);

    private:
        SubBufferBase _buffer;
        void* _mapping = nullptr;
        MappingAccess _access = MappingAccess::ReadWrite;
};

template<typename Elem>
class BufferMapping final : public BufferMappingBase {
    public:
        using iterator = Elem* ;
        using const_iterator = Elem const*;
        using value_type = Elem;

        u64 size() const {
            return this->byte_size() / sizeof(Elem);
        }

        iterator begin() {
            return static_cast<iterator>(BufferMappingBase::raw_data());
        }

        iterator end() {
            return begin() + size();
        }

        const_iterator begin() const {
            return static_cast<const_iterator>(BufferMappingBase::raw_data());
        }

        const_iterator end() const {
            return begin() + size();
        }

        const_iterator cbegin() const {
            return begin();
        }

        const_iterator cend() const {
            return end();
        }

        value_type& operator[](usize i) {
            y_debug_assert(i < size());
            return begin()[i];
        }

        const value_type& operator[](usize i) const{
            y_debug_assert(i < size());
            return begin()[i];
        }

        value_type& operator*() {
            return *data();
        }

        const value_type& operator*() const{
            return *data();
        }

        value_type* operator->() {
            return data();
        }

        const value_type* operator->() const{
            return data();
        }

        value_type* data() {
            return begin();
        }

        const value_type* data() const {
            return begin();
        }

    private:
        template<typename T, typename B>
        friend class TypedWrapper;

        template<BufferUsage U, MemoryType M>
        friend class Buffer;

        template<BufferUsage U, MemoryType M>
        friend class SubBuffer;

        BufferMapping(const SubBuffer<BufferUsage::None, MemoryType::CpuVisible>& buffer, MappingAccess access) : BufferMappingBase(buffer, access) {
        }
};


}

#endif // YAVE_GRAPHICS_BUFFERS_MAPPING_H

