/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef YAVE_BUFFERS_BUFFERUSAGE_H
#define YAVE_BUFFERS_BUFFERUSAGE_H

#include <yave/vk/vk.h>

namespace yave {

enum class BufferUsage {
	None = 0,
	AttributeBit = int(vk::BufferUsageFlagBits::eVertexBuffer),
	IndexBit = int(vk::BufferUsageFlagBits::eIndexBuffer),
	IndirectBit = int(vk::BufferUsageFlagBits::eIndirectBuffer),
	UniformBit = int(vk::BufferUsageFlagBits::eUniformBuffer),
	StorageBit = int(vk::BufferUsageFlagBits::eStorageBuffer)
};

constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) {
	return BufferUsage(uenum(a) | uenum(b));
}

constexpr BufferUsage operator&(BufferUsage a, BufferUsage b) {
	return BufferUsage(uenum(a) & uenum(b));
}



enum class BufferTransfer {
	None = 0,
    TransferSrc = int(vk::BufferUsageFlagBits::eTransferSrc),
    TransferDst = int(vk::BufferUsageFlagBits::eTransferDst)
};

constexpr BufferTransfer operator|(BufferTransfer a, BufferTransfer b) {
	return BufferTransfer(uenum(a) | uenum(b));
}

constexpr BufferTransfer operator&(BufferTransfer a, BufferTransfer b) {
	return BufferTransfer(uenum(a) & uenum(b));
}




// Y_TODO(maybe ditch eHostCoherent for a raii flush)
enum class MemoryFlags {
    DeviceLocal = uenum(vk::MemoryPropertyFlagBits::eDeviceLocal),
	CpuVisible = uenum(vk::MemoryPropertyFlagBits::eHostVisible)
};




inline constexpr bool is_cpu_visible(MemoryFlags flags) {
	return uenum(flags) & uenum(vk::MemoryPropertyFlagBits::eHostVisible);
}



inline constexpr MemoryFlags prefered_memory_flags(BufferUsage usage) {
	return ((usage & (BufferUsage::UniformBit | BufferUsage::StorageBit)) != BufferUsage::None) ? MemoryFlags::CpuVisible : MemoryFlags::DeviceLocal;
}

inline constexpr BufferTransfer prefered_transfer(MemoryFlags flags) {
	return is_cpu_visible(flags) ? BufferTransfer::None : BufferTransfer::TransferDst;
}




}

#endif // YAVE_BUFFERS_BUFFERUSAGE_H
