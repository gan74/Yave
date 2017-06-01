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

#include "StagingBufferMapping.h"

#include <yave/commands/CmdBufferRecorder.h>
#include <yave/device/Device.h>

namespace yave {

static auto create_src_buffer(DevicePtr dptr, usize size) {
	return StagingBufferMapping::StagingBuffer(dptr, size);
}


StagingBufferMapping::StagingBufferMapping(const SubBufferBase& dst) :
		CpuVisibleMapping(),
		_dst(dst),
		_src(create_src_buffer(dst.device(), dst.byte_size())) {

	CpuVisibleMapping cpu_mapping(_src);
	CpuVisibleMapping::swap(cpu_mapping);
}

void StagingBufferMapping::swap(StagingBufferMapping& other) {
	CpuVisibleMapping::swap(other);
	std::swap(_dst, other._dst);
	std::swap(_src, other._src);
}

vk::BufferCopy StagingBufferMapping::vk_copy() const {
	return vk::BufferCopy()
			.setDstOffset(_dst.byte_offset())
			.setSrcOffset(0)
			.setSize(_dst.byte_size());
		;
}

StagingBufferMapping::~StagingBufferMapping() {
	CpuVisibleMapping done;
	CpuVisibleMapping::swap(done);

	DevicePtr device = _dst.device();
	auto transfer_queue = device->vk_queue(QueueFamily::Graphics);

	auto transfer_cmd_buffer = CmdBufferRecorder<CmdBufferUsage::Disposable>(device->create_disposable_cmd_buffer());
	//transfer_cmd_buffer.copy_buffer(_dst_ref, BufferMemoryReference<MemoryFlags::CpuVisible, BufferTransfer::TransferSrc>(_src));
	transfer_cmd_buffer.vk_cmd_buffer().copyBuffer(_src.vk_buffer(), _dst.vk_buffer(), vk_copy());
	transfer_cmd_buffer.end().submit<SyncSubmit>(transfer_queue);
}


}
