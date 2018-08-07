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

#include "CmdBufferBase.h"

#include <yave/graphics/commands/pool/CmdBufferPoolBase.h>
#include <yave/device/Device.h>

namespace yave {

CmdBufferBase::CmdBufferBase(std::shared_ptr<CmdBufferDataProxy>&& data) : _proxy(std::move(data)) {
}

CmdBufferBase::CmdBufferBase(CmdBufferBase&& other) {
	swap(other);
}

void CmdBufferBase::swap(CmdBufferBase& other) {
	std::swap(_proxy, other._proxy);
}

void CmdBufferBase::wait() const {
	device()->vk_device().waitForFences({vk_fence()}, true, u64(-1));
}

DevicePtr CmdBufferBase::device() const {
	auto pool = _proxy->data().pool();
	return pool ? pool->device() : nullptr;
}

vk::CommandBuffer CmdBufferBase::vk_cmd_buffer() const {
	return _proxy ? _proxy->data().vk_cmd_buffer() : vk::CommandBuffer();
}

vk::Fence CmdBufferBase::vk_fence() const {
	return  _proxy ? _proxy->data().vk_fence() : vk::Fence();
}

}
