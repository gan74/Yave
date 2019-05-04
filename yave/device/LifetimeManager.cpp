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

namespace yave {

// https://stackoverflow.com/questions/16190078/how-to-atomically-update-a-maximum-value
template<typename T>
static void update_maximum(std::atomic<T>& max, const T& val) {
	T prev = max;
	while(prev < val && !max.compare_exchange_weak(prev, val));
}


LifetimeManager::LifetimeManager(DevicePtr dptr) : DeviceLinked(dptr) {
}

LifetimeManager::~LifetimeManager() {
	for(auto& r : _to_destroy) {
		detail::destroy(device(), r.second);
	}
}

ResourceFence LifetimeManager::create_fence() {
	return ++_fence;
}

ResourceFence LifetimeManager::recycle_fence(ResourceFence fence) {
	if(ResourceFence(_done_fence) > fence) {
		update_maximum(_done_fence, fence._value);
		clear_resources(_done_fence);
	}
	return create_fence();
}

void LifetimeManager::clear_resources(u64 up_to) {
	y_profile();
	std::unique_lock lock(_lock);
	while(!_to_destroy.empty() && _to_destroy.front().first <= up_to) {
		detail::destroy(device(), _to_destroy.front().second);
		_to_destroy.pop_front();
	}
}


}
