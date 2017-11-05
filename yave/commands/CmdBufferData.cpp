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
		_cmd_buffer(buf), _fence(fen), _pool(p) {
}

CmdBufferData::CmdBufferData(CmdBufferData&& other) {
	swap(other);
}

CmdBufferData& CmdBufferData::operator=(CmdBufferData&& other) {
	swap(other);
	return *this;
}

CmdBufferData::~CmdBufferData() {
	if(_pool) {
		if(_fence && device()->vk_device().waitForFences({_fence}, true, 0) != vk::Result::eSuccess) {
			fatal("CmdBuffer is still in use.");
		}
		device()->vk_device().freeCommandBuffers(_pool->vk_pool(), _cmd_buffer);
		device()->destroy(_fence);
	}
}

DevicePtr CmdBufferData::device() const {
	return _pool ? _pool->device() : nullptr;
}

CmdBufferPoolBase* CmdBufferData::pool() const {
	return _pool;
}

const vk::CommandBuffer& CmdBufferData::vk_cmd_buffer() const {
	return _cmd_buffer;
}

const vk::Fence& CmdBufferData::vk_fence() const {
	return _fence;
}

void CmdBufferData::swap(CmdBufferData& other) {
	std::swap(_cmd_buffer, other._cmd_buffer);
	std::swap(_fence, other._fence);
	std::swap(_keep_alive, other._keep_alive);
	std::swap(_pool, other._pool);
}

void CmdBufferData::reset() {
	_keep_alive.clear();
	_cmd_buffer.reset(vk::CommandBufferResetFlags());
}

bool CmdBufferData::try_reset() {
	if(_fence && device()->vk_device().waitForFences({_fence}, true, 0) != vk::Result::eSuccess) {
		return false;
	}
	reset();
	return true;
}

CmdBufferDataProxy::CmdBufferDataProxy(CmdBufferDataProxy&& other) {
	std::swap(_data, other._data);
}

CmdBufferDataProxy& CmdBufferDataProxy::operator=(CmdBufferDataProxy&& other) {
	std::swap(_data, other._data);
	return *this;
}

CmdBufferDataProxy::CmdBufferDataProxy(CmdBufferData&& d) : _data(std::move(d)) {
}

CmdBufferDataProxy::~CmdBufferDataProxy() {
	if(_data.pool()) {
		_data.pool()->release(std::move(_data));
	}
}

CmdBufferData& CmdBufferDataProxy::data() {
	return _data;
}

}
