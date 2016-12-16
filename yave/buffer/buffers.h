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
	UniformBuffer = int(vk::BufferUsageFlagBits::eUniformBuffer)
};

enum class BufferTransfer {
	None = 0,
    TransferSrc = int(vk::BufferUsageFlagBits::eTransferSrc),
    TransferDst = int(vk::BufferUsageFlagBits::eTransferDst)
};


Y_TODO(ditch eHostCoherent for a raii flush)
enum class MemoryFlags {
    DeviceLocal = uenum(vk::MemoryPropertyFlagBits::eDeviceLocal),
	CpuVisible = uenum(vk::MemoryPropertyFlagBits::eHostVisible) | uenum(vk::MemoryPropertyFlagBits::eHostCoherent)
};

template<MemoryFlags Flags>
static constexpr bool is_cpu_visible_v = uenum(Flags) & uenum(vk::MemoryPropertyFlagBits::eHostVisible);

bool is_cpu_visible(MemoryFlags flags);


template<BufferUsage Usage>
struct PreferedMemoryFlags {
	static constexpr MemoryFlags value = MemoryFlags::DeviceLocal;
};

template<>
struct PreferedMemoryFlags<BufferUsage::UniformBuffer> {
	static constexpr MemoryFlags value = MemoryFlags::CpuVisible;
};


template<MemoryFlags Flags>
struct PreferedBufferTransfer {
	static constexpr BufferTransfer value = BufferTransfer::TransferDst;
};

template<>
struct PreferedBufferTransfer<MemoryFlags::CpuVisible> {
	static constexpr BufferTransfer value = BufferTransfer::None;
};



}

#endif // YAVE_BUFFER_BUFFERS_H
