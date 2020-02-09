/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
		_cmd_buffer(buf), _fence(fen), _pool(p), _resource_fence(device()->lifetime_manager().create_fence()) {
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
		if(_fence && device()->vk_device().getFenceStatus(_fence) != vk::Result::eSuccess) {
			y_fatal("CmdBuffer is still in use.");
		}
		device()->vk_device().freeCommandBuffers(_pool->vk_pool(), _cmd_buffer);
		device()->destroy(_fence);
	}
}

DevicePtr CmdBufferData::device() const {
	return _pool ? _pool->device() : nullptr;
}

bool CmdBufferData::is_null() const {
	return !device();
}

CmdBufferPoolBase* CmdBufferData::pool() const {
	return _pool;
}

vk::CommandBuffer CmdBufferData::vk_cmd_buffer() const {
	return _cmd_buffer;
}

vk::Fence CmdBufferData::vk_fence() const {
	return _fence;
}

ResourceFence CmdBufferData::resource_fence() const {
	return _resource_fence;
}

void CmdBufferData::swap(CmdBufferData& other) {
	std::swap(_cmd_buffer, other._cmd_buffer);
	std::swap(_fence, other._fence);
	std::swap(_keep_alive, other._keep_alive);
	std::swap(_pool, other._pool);
	std::swap(_signal, other._signal);
	std::swap(_waits, other._waits);
	std::swap(_resource_fence, other._resource_fence);
}

void CmdBufferData::reset() {
	y_profile();
	if(_fence) {
		device()->vk_device().resetFences({_fence});
	}

	_cmd_buffer.reset(vk::CommandBufferResetFlags());
	_waits.clear();
	_signal = Semaphore();
	_resource_fence = device()->lifetime_manager().create_fence();
}

void CmdBufferData::release_resources() {
	_keep_alive.clear();
}

void CmdBufferData::wait_for(const Semaphore& sem) {
	if(sem.device() && std::find(_waits.begin(), _waits.end(), sem) == _waits.end()) {
		_waits.emplace_back(sem);
	}
}



CmdBufferDataProxy::CmdBufferDataProxy(CmdBufferData&& d) : _data(std::move(d)) {
}

CmdBufferDataProxy::~CmdBufferDataProxy() {
	if(!_data.is_null()) {
		_data.device()->lifetime_manager().recycle(std::move(_data));
		//_data.pool()->release(std::move(_data));
	}
}

CmdBufferData& CmdBufferDataProxy::data() {
	return _data;
}

}
