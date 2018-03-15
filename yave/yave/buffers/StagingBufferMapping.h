/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef YAVE_BUFFERS_STAGINGBUFFERMAPPING_H
#define YAVE_BUFFERS_STAGINGBUFFERMAPPING_H

#include "BufferUsage.h"
#include "CpuVisibleMapping.h"

namespace yave {

class CmdBufferRecorderBase;

class StagingBufferMapping : public CpuVisibleMapping {

	public:
		using staging_buffer_type = Buffer<BufferUsage::None, MemoryType::CpuVisible, BufferTransfer::TransferSrc>;

		StagingBufferMapping() = default;
		StagingBufferMapping(const SubBuffer<BufferUsage::None, MemoryType::DontCare, BufferTransfer::TransferDst>& dst, CmdBufferRecorderBase& recorder);

		template<BufferUsage Usage, MemoryType Memory>
		StagingBufferMapping(Buffer<Usage, Memory, BufferTransfer::TransferDst>& buffer, CmdBufferRecorderBase& recorder) : StagingBufferMapping(SubBuffer(buffer), recorder) {
		}

		~StagingBufferMapping();

	protected:
		void swap(StagingBufferMapping& other);

	private:
		vk::BufferCopy vk_copy() const;

		CmdBufferRecorderBase* _cmd_buffer = nullptr;
		SubBufferBase _dst;
		staging_buffer_type _src;
};

}

#endif // YAVE_BUFFERS_STAGINGBUFFERMAPPING_H
