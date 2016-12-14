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
#ifndef YAVE_BUFFER_STAGINGBUFFERMAPPING_H
#define YAVE_BUFFER_STAGINGBUFFERMAPPING_H

#include "buffers.h"
#include "CpuVisibleMapping.h"

namespace yave {

class StagingBufferMapping : public CpuVisibleMapping {

	public:
		using StagingBuffer = Buffer<BufferUsage::None, MemoryFlags::CpuVisible, BufferTransfer::TransferSrc>;

		template<BufferUsage Usage>
		StagingBufferMapping(SubBuffer<Usage, MemoryFlags::DeviceLocal, BufferTransfer::TransferDst>& buffer) : StagingBufferMapping(SubBufferBase(buffer)) {
		}

		template<BufferUsage Usage>
		StagingBufferMapping(Buffer<Usage, MemoryFlags::DeviceLocal, BufferTransfer::TransferDst>& buffer) : StagingBufferMapping(SubBufferBase(buffer)) {
		}

		StagingBufferMapping() = default;
		~StagingBufferMapping();

	protected:
		void swap(StagingBufferMapping& other);

	private:
		StagingBufferMapping(const SubBufferBase& dst);

		vk::BufferCopy vk_copy() const;

		SubBufferBase _dst;
		StagingBuffer _src;
};

}

#endif // YAVE_BUFFER_STAGINGBUFFERMAPPING_H
