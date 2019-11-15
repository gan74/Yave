/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
	y_debug_assert(_counter == _done_counter);
	for(auto& r : _to_destroy) {
		/*std::visit([](auto& r) {
			log_msg(type_name(r));
			if constexpr(std::is_same_v<remove_cvref_t<decltype(r)>, DescriptorSetData>) {
				y_breakpoint;
			}
		}, r.second);*/

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

	bool run_collect = false;
	{
		const std::unique_lock lock(_cmd_lock);
		const auto it = std::lower_bound(_in_flight.begin(), _in_flight.end(), cmd,
								   [](const auto& a, const auto& b) { return a.resource_fence() < b.resource_fence(); });
		_in_flight.insert(it, std::move(cmd));
		run_collect = (_in_flight.front().resource_fence()._value == _done_counter + 1);
	}

	if(run_collect) {
		collect();
	}
}

void LifetimeManager::collect() {
	y_profile();

	u64 next = 0;
	bool clear = false;

	{
		const std::unique_lock lock(_cmd_lock);
		next = _done_counter;
		while(!_in_flight.empty()) {
			CmdBufferData& cmd = _in_flight.front();
			const u64 fence = cmd.resource_fence()._value;
			if(fence != next + 1) {
				break;
			}

			if(device()->vk_device().getFenceStatus(cmd.vk_fence()) == vk::Result::eSuccess) {
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
			clear = true;
			_done_counter = next;
		}
	}

	if(clear) {
		clear_resources(next);
	}
}

usize LifetimeManager::pending_deletions() const {
	const std::unique_lock lock(_resource_lock);
	return _to_destroy.size();
}

usize LifetimeManager::active_cmd_buffers() const {
	const std::unique_lock lock(_cmd_lock);
	return _in_flight.size();
}

void LifetimeManager::destroy_resource(ManagedResource& resource) const {
	std::visit(
		[dptr = device()](auto& res) {
			if constexpr(std::is_same_v<decltype(res), DeviceMemory&>) {
				res.free();
			} else if constexpr(std::is_same_v<decltype(res), DescriptorSetData&>) {
				res.recycle();
			} else {
				detail::destroy(dptr, res);
			}
		},
		resource);
}

void LifetimeManager::clear_resources(u64 up_to) {
	y_profile();
	core::Vector<ManagedResource> to_del;

	{
		const std::unique_lock lock(_resource_lock);
		while(!_to_destroy.empty() && _to_destroy.front().first <= up_to) {
			to_del << std::move(_to_destroy.front().second);
			_to_destroy.pop_front();
		}
	}

	for(auto& res : to_del) {
		destroy_resource(res);
	}
}


}
