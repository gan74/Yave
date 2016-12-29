/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef YAVE_BUFFER_BUFFERS_H
#define YAVE_BUFFER_BUFFERS_H

#include <yave/yave.h>

namespace yave {

enum class BufferUsage {
	None = 0,
    VertexBuffer = int(vk::BufferUsageFlagBits::eVertexBuffer),
	IndexBuffer = int(vk::BufferUsageFlagBits::eIndexBuffer),
	IndirectBuffer = int(vk::BufferUsageFlagBits::eIndirectBuffer),
	UniformBuffer = int(vk::BufferUsageFlagBits::eUniformBuffer),
	StorageBuffer = int(vk::BufferUsageFlagBits::eStorageBuffer)
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




Y_TODO(ditch eHostCoherent for a raii flush)
enum class MemoryFlags {
    DeviceLocal = uenum(vk::MemoryPropertyFlagBits::eDeviceLocal),
	CpuVisible = uenum(vk::MemoryPropertyFlagBits::eHostVisible) | uenum(vk::MemoryPropertyFlagBits::eHostCoherent)
};




template<MemoryFlags Flags>
inline constexpr bool is_cpu_visible() {
	return uenum(Flags) & uenum(vk::MemoryPropertyFlagBits::eHostVisible);
}

inline bool is_cpu_visible(MemoryFlags flags) {
	return uenum(flags) & uenum(vk::MemoryPropertyFlagBits::eHostVisible);
}



template<BufferUsage Usage>
inline constexpr MemoryFlags prefered_memory_flags() {
	return ((Usage & (BufferUsage::UniformBuffer | BufferUsage::StorageBuffer)) != BufferUsage::None) ? MemoryFlags::CpuVisible : MemoryFlags::DeviceLocal;
}

template<MemoryFlags Flags>
inline constexpr BufferTransfer prefered_transfer() {
	return is_cpu_visible<Flags>() ? BufferTransfer::None : BufferTransfer::TransferDst;
}




}

#endif // YAVE_BUFFER_BUFFERS_H
