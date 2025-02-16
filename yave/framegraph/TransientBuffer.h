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
#ifndef YAVE_FRAMEGRAPH_TRANSIENTBUFFER_H
#define YAVE_FRAMEGRAPH_TRANSIENTBUFFER_H

#include <yave/graphics/buffers/Buffer.h>

namespace yave {

class TransientBuffer final : public BufferBase {

    static constexpr MemoryType memory_type(MemoryType memory, BufferUsage usage) {
        return memory == MemoryType::DontCare ? prefered_memory_type(usage) : memory;
    }

    public:
        TransientBuffer() = default;

        TransientBuffer(usize byte_size, BufferUsage usage, MemoryType type = MemoryType::DontCare) :
                BufferBase(byte_size, usage, memory_type(type, usage)),
                _memory_type(type) {
            set_exposed_byte_size(byte_size);
        }

        MemoryType memory_type() const {
            return _memory_type;
        }

        u64 exposed_byte_size() const {
            return _exposed_byte_size;
        }

        void set_exposed_byte_size(u64 size) {
            y_debug_assert(size <= byte_size());
            _exposed_byte_size = size;
        }

    private:
        MemoryType _memory_type = MemoryType::DeviceLocal;
        u64 _exposed_byte_size = 0;
};


template<BufferUsage Usage, MemoryType Memory = MemoryType::DontCare>
class TransientSubBuffer final : public SubBuffer<Usage, Memory> {
    public:
        TransientSubBuffer(const TransientBuffer& buffer) : SubBuffer<Usage, Memory>(SubBufferBase(buffer, buffer.exposed_byte_size(), 0)) {
            y_debug_assert(this->byte_size() == buffer.exposed_byte_size() && this->byte_offset() == 0);

            if(!this->has(buffer.usage(), Usage)) {
                y_fatal("Invalid subbuffer usage");
            }
            if(!is_memory_type_compatible(buffer.memory_type(), Memory)) {
                y_fatal("Invalid subbuffer memory type");
            }
        }
};

}

#endif // YAVE_FRAMEGRAPH_TRANSIENTBUFFER_H

