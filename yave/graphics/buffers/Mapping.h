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
#ifndef YAVE_GRAPHICS_BUFFERS_MAPPING_H
#define YAVE_GRAPHICS_BUFFERS_MAPPING_H

#include "SubBuffer.h"

namespace yave {

class CmdBufferRecorder;

class Mapping : NonMovable {

    public:
        Mapping() = default;

        template<BufferUsage Usage>
        Mapping(const Buffer<Usage, MemoryType::CpuVisible>& buffer) : Mapping(SubBuffer<BufferUsage::None, MemoryType::CpuVisible>(buffer)) {
        }

        Mapping(const SubBuffer<BufferUsage::None, MemoryType::CpuVisible>& buffer);

        static void stage(const SubBuffer<BufferUsage::TransferDstBit>& dst, CmdBufferRecorder& recorder, const void* data);

        ~Mapping();

        // No need to barrier after flush
        void flush();

        void* data();
        const void* data() const;

        usize byte_size() const;

    protected:
        void swap(Mapping& other);

    private:
        SubBufferBase _buffer;
        void* _mapping = nullptr;
};

}

#endif // YAVE_GRAPHICS_BUFFERS_MAPPING_H

