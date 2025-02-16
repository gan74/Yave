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
#ifndef YAVE_GRAPHICS_BUFFERS_BUFFERUSAGE_H
#define YAVE_GRAPHICS_BUFFERS_BUFFERUSAGE_H

#include <yave/graphics/vk/vk.h>
#include <yave/graphics/memory/MemoryType.h>

namespace yave {

enum class BufferUsage : u32 {
    None = 0,

    AttributeBit                = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
    IndexBit                    = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
    IndirectBit                 = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
    UniformBit                  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    StorageBit                  = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,

    TransferSrcBit              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    TransferDstBit              = VK_BUFFER_USAGE_TRANSFER_DST_BIT,

    AccelStructureInputBit      = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
    AccelStructureBit           = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
    BindingTableBit             = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
};

constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return BufferUsage(u32(a) | u32(b));
}

constexpr BufferUsage operator&(BufferUsage a, BufferUsage b) {
    return BufferUsage(u32(a) & u32(b));
}

inline constexpr MemoryType prefered_memory_type(BufferUsage) {
    return MemoryType::DeviceLocal;
}

inline constexpr BufferUsage buffer_transfer_usage(MemoryType flags) {
    return is_cpu_visible(flags) ? BufferUsage::None : BufferUsage::TransferDstBit;
}

}

#endif // YAVE_GRAPHICS_BUFFERS_BUFFERUSAGE_H

