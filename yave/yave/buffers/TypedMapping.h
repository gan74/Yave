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
#ifndef YAVE_BUFFERS_TYPEDMAPPING_H
#define YAVE_BUFFERS_TYPEDMAPPING_H

#include "BufferUsage.h"
#include "TypedSubBuffer.h"
#include "StagingBufferMapping.h"

#include <yave/device/Device.h>
#include <yave/queues/QueueFamily.h>
#include <yave/queues/Queue.h>

namespace yave {

template<MemoryType Memory>
using MemoryMapping = std::conditional_t<is_cpu_visible(Memory), CpuVisibleMapping, StagingBufferMapping>;

template<typename Elem, MemoryType Memory>
class TypedMapping : public MemoryMapping<Memory> {

	using Base = MemoryMapping<Memory>;

	public:
		using iterator = Elem* ;
		using const_iterator = Elem const* ;
		using value_type = Elem;

		template<BufferUsage Usage, BufferTransfer Transfer>
		explicit TypedMapping(TypedBuffer<Elem, Usage, Memory, Transfer>& buffer) : Base(buffer) {
			static_assert(!require_staging(Memory), "Buffer requires staging");
		}

		template<BufferUsage Usage, BufferTransfer Transfer>
		explicit TypedMapping(TypedSubBuffer<Elem, Usage, Memory, Transfer>& buffer) : Base(buffer) {
			static_assert(!require_staging(Memory), "SubBuffer requires staging");
		}

		template<BufferUsage Usage>
		explicit TypedMapping(TypedBuffer<Elem, Usage, Memory, BufferTransfer::TransferDst>& buffer, CmdBufferRecorderBase& recorder) : Base(buffer, recorder) {
			static_assert(require_staging(Memory), "Buffer does not require staging");
		}

		template<BufferUsage Usage>
		explicit TypedMapping(TypedSubBuffer<Elem, Usage, Memory, BufferTransfer::TransferDst>& buffer, CmdBufferRecorderBase& recorder) : Base(buffer, recorder) {
			static_assert(require_staging(Memory), "SubBuffer does not require staging");
		}

		TypedMapping(TypedMapping&& other) {
			swap(other);
		}

		TypedMapping& operator=(TypedMapping&& other) {
			swap(other);
			return *this;
		}

		usize size() const {
			return this->byte_size() / sizeof(Elem);
		}

		iterator begin() {
			return reinterpret_cast<iterator>(this->data());
		}

		iterator end() {
			return begin() + size();
		}

		const_iterator begin() const {
			return reinterpret_cast<const_iterator>(this->data());
		}

		const_iterator end() const {
			return begin() + size();
		}

		const_iterator cbegin() const {
			return begin();
		}

		const_iterator cend() const {
			return end();
		}

		value_type& operator[](usize i) {
			return begin()[i];
		}

		const value_type& operator[](usize i) const{
			return begin()[i];
		}

	private:

};

template<typename Elem, BufferUsage Usage, MemoryType Memory, BufferTransfer Transfer>
TypedBuffer<Elem, Usage, Memory, Transfer>::TypedBuffer(DevicePtr dptr, const core::ArrayView<Elem>& data) : TypedBuffer(dptr, data.size()) {
	if constexpr(require_staging(Memory)) {
		Y_LOG_PERF("buffer,staging");
		CmdBufferRecorder recorder(dptr->create_disposable_cmd_buffer());
		std::copy(data.begin(), data.end(), TypedMapping(*this, recorder).begin());
		dptr->queue(QueueFamily::Graphics).submit<SyncSubmit>(RecordedCmdBuffer<CmdBufferUsage::Disposable>(std::move(recorder)));
	} else {
		std::copy(data.begin(), data.end(), TypedMapping(*this).begin());
	}
}

template<typename Elem, BufferUsage Usage, MemoryType Memory, BufferTransfer Transfer>
TypedMapping(TypedBuffer<Elem, Usage, Memory, Transfer>&) -> TypedMapping<Elem, Memory>;
template<typename Elem, BufferUsage Usage, MemoryType Memory, BufferTransfer Transfer>
TypedMapping(TypedBuffer<Elem, Usage, Memory, Transfer>&, CmdBufferRecorderBase&) -> TypedMapping<Elem, Memory>;
template<typename Elem, BufferUsage Usage, MemoryType Memory, BufferTransfer Transfer>
TypedMapping(TypedSubBuffer<Elem, Usage, Memory, Transfer>&) -> TypedMapping<Elem, Memory>;
template<typename Elem, BufferUsage Usage, MemoryType Memory, BufferTransfer Transfer>
TypedMapping(TypedSubBuffer<Elem, Usage, Memory, Transfer>&, CmdBufferRecorderBase&) -> TypedMapping<Elem, Memory>;


}

#endif // YAVE_BUFFERS_TYPEDMAPPING_H
