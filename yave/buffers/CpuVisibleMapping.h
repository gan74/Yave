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
#ifndef YAVE_BUFFERS_CPUVISIBLEMAPPING_H
#define YAVE_BUFFERS_CPUVISIBLEMAPPING_H

#include "SubBuffer.h"

namespace yave {

class CpuVisibleMapping : NonCopyable {

	public:
		CpuVisibleMapping() = default;

		template<BufferUsage Usage, BufferTransfer Transfer>
		CpuVisibleMapping(const SpecializedSubBuffer<Usage, MemoryFlags::CpuVisible, Transfer>& buffer) : CpuVisibleMapping(SubBufferBase(buffer)) {
		}

		template<BufferUsage Usage, BufferTransfer Transfer>
		CpuVisibleMapping(const Buffer<Usage, MemoryFlags::CpuVisible, Transfer>& buffer) : CpuVisibleMapping(SubBufferBase(buffer)) {
		}

		CpuVisibleMapping(CpuVisibleMapping&& other);
		CpuVisibleMapping& operator=(CpuVisibleMapping&& other);

		~CpuVisibleMapping();

		void flush();

		void* data();
		const void* data() const;
		usize byte_size() const;

	protected:
		void swap(CpuVisibleMapping& other);

	private:
		CpuVisibleMapping(const SubBufferBase& buff);

		SubBufferBase _buffer;
		void* _mapping = nullptr;
};

}

#endif // YAVE_BUFFERS_CPUVISIBLEMAPPING_H
