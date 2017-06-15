/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "CmdBufferData.h"
#include <yave/device/Device.h>

namespace yave {

CmdBufferData::CmdBufferData(vk::CommandBuffer buf, vk::Fence fen, CmdBufferPoolBase* p) :
		cmd_buffer(buf), fence(fen), pool(p) {
}

CmdBufferData::CmdBufferData(CmdBufferData&& other) {
	swap(other);
}

CmdBufferData& CmdBufferData::operator=(CmdBufferData&& other) {
	swap(other);
	return *this;
}

void CmdBufferData::swap(CmdBufferData& other) {
	std::swap(cmd_buffer, other.cmd_buffer);
	std::swap(fence, other.fence);
	std::swap(dependencies, other.dependencies);
	std::swap(pool, other.pool);
}

bool CmdBufferData::wait_for_fences(u64 timeout) const {
	if(!pool) {
		return true;
	}

	auto dptr = pool->device();
	if(!dependencies.is_empty()) {
		vk::ArrayProxy<const vk::Fence> fences(u32(dependencies.size()), dependencies.begin());
		if(dptr->vk_device().waitForFences(fences, true, timeout) != vk::Result::eSuccess) {
			return false;
		}
	}
	if(fence) {
		if(dptr->vk_device().waitForFences(fence, true, timeout) != vk::Result::eSuccess) {
			return false;
		}
	}
	return true;
}

}
