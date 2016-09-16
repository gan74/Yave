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

#include "StagingBufferMapping.h"

#include <yave/command/CmdBufferRecorder.h>
#include <yave/Device.h>

namespace yave {

static auto create_src_buffer(DevicePtr dptr, usize size) {
	return StagingBufferMapping::StagingBuffer(dptr, size);
}


StagingBufferMapping::StagingBufferMapping(DstBufferRef dst) :
		CpuVisibleMapping(),
		_dst_ref(dst),
		_src(create_src_buffer(dst->get_device(), dst->byte_size())) {
	CpuVisibleMapping cpu_mapping(_src);
	CpuVisibleMapping::swap(cpu_mapping);
}

void StagingBufferMapping::swap(StagingBufferMapping& other) {
	CpuVisibleMapping::swap(other);
	std::swap(_dst_ref, other._dst_ref);
	std::swap(_src, other._src);
}

StagingBufferMapping::~StagingBufferMapping() {
	CpuVisibleMapping done;
	CpuVisibleMapping::swap(done);

	DevicePtr device = _dst_ref->get_device();
	auto transfer_queue = device->get_vk_queue(QueueFamily::Graphics);

	auto transfer_cmd_buffer = CmdBufferRecorder(device->create_disposable_command_buffer());
	transfer_cmd_buffer.copy_buffer(_dst_ref, BufferMemoryReference<MemoryFlags::CpuVisible, BufferTransfer::TransferSrc>(_src));
	transfer_cmd_buffer.end().submit(transfer_queue);

	Y_TODO(buffer transfer stall)
	transfer_queue.waitIdle();
}


}
