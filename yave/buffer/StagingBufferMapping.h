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
#ifndef YAVE_BUFFER_STAGINGBUFFERMAPPING_H
#define YAVE_BUFFER_STAGINGBUFFERMAPPING_H

#include "BufferUsage.h"
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
