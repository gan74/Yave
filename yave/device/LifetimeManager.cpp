/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "LifetimeManager.h"
#include "Device.h"

#include <yave/graphics/commands/data/CmdBufferData.h>
#include <yave/graphics/commands/pool/CmdBufferPoolBase.h>

#include <yave/graphics/vk/destroy.h>

namespace yave {

LifetimeManager::LifetimeManager(DevicePtr dptr) : DeviceLinked(dptr) {
}

LifetimeManager::~LifetimeManager() {
	std::unique_lock lock(_lock);
	for(auto& r : _to_destroy) {
		destroy_resource(r.second);
	}
	if(_in_flight.size()) {
		y_fatal("% CmdBuffer still in flight.", _in_flight.size());
	}
}

ResourceFence LifetimeManager::create_fence() {
	return ++_counter;
}

void LifetimeManager::recycle(CmdBufferData&& cmd) {
	y_profile();
	std::unique_lock lock(_lock);
	auto it = std::lower_bound(_in_flight.begin(), _in_flight.end(), cmd,
								[](const auto& a, const auto& b) { return a.resource_fence() < b.resource_fence(); });
	_in_flight.insert(it, std::move(cmd));

	if(_in_flight.front().resource_fence()._value == _done_counter + 1) {
		collect();
	}

	y_debug_assert(_in_flight.size() < 256);
}

void LifetimeManager::collect() {
	y_profile();
	u64 next = _done_counter;
	while(!_in_flight.empty()) {
		CmdBufferData& cmd = _in_flight.front();
		u64 fence = cmd.resource_fence()._value;
		auto status = vk::Result::eNotReady;
		{
			y_profile_zone("Fence status");
			status = device()->vk_device().getFenceStatus(cmd.vk_fence());
		}
		if(fence == next + 1 && status == vk::Result::eSuccess) {
			if(cmd.pool()) {
				cmd.pool()->release(std::move(cmd));
			}

			next = fence;
			_in_flight.pop_front();
		} else {
			break;
		}
	}
	if(next != _done_counter) {
		clear_resources(_done_counter = next);
	}
}

usize LifetimeManager::pending_deletions() const {
	std::unique_lock lock(_lock);
	return _to_destroy.size();
}

usize LifetimeManager::active_cmd_buffers() const {
	std::unique_lock lock(_lock);
	return _in_flight.size();
}

void LifetimeManager::destroy_resource(ManagedResource& resource) const {
	y_profile();
	std::visit(
		[dptr = device()](auto& res) {
			if constexpr(std::is_same_v<decltype(res), DeviceMemory&>) {
				y_profile_zone("Memory");
				res.free();
			} else if constexpr(std::is_same_v<decltype(res), DescriptorSetData&>) {
				y_profile_zone("Descriptor set");
				res.recycle();
			} else {
				y_profile_zone("Vk resource");
				detail::destroy(dptr, res);
			}
		},
		resource);
}

void LifetimeManager::clear_resources(u64 up_to) {
	y_profile();
	while(!_to_destroy.empty() && _to_destroy.front().first <= up_to) {
		destroy_resource(_to_destroy.front().second);
		_to_destroy.pop_front();
	}
}


}
