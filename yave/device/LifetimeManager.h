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
#ifndef YAVE_DEVICE_LIFETIMEMANAGER_H
#define YAVE_DEVICE_LIFETIMEMANAGER_H

#include "DeviceLinked.h"

#include <y/concurrent/SpinLock.h>
#include <yave/graphics/vk/destroy.h>

#include <mutex>
#include <deque>

namespace yave {

class ResourceFence {
	public:
		ResourceFence() = default;


		bool operator==(const ResourceFence& other) const {
			return _value == other._value;
		}

		bool operator!=(const ResourceFence& other) const {
			return _value != other._value;
		}


		bool operator<(const ResourceFence& other) const {
			return _value < other._value;
		}

		bool operator<=(const ResourceFence& other) const {
			return _value <= other._value;
		}


		bool operator>(const ResourceFence& other) const {
			return _value > other._value;
		}

		bool operator>=(const ResourceFence& other) const {
			return _value >= other._value;
		}


	private:
		friend class LifetimeManager;

		ResourceFence(u32 v) : _value(v) {
		}

		u64 _value = 0;

};

class LifetimeManager : NonCopyable, public DeviceLinked {
	public:
		LifetimeManager(DevicePtr dptr);
		~LifetimeManager();

		ResourceFence create_fence();
		[[nodiscard]] ResourceFence recycle_fence(ResourceFence fence);

		template<typename T>
		void destroy_immediate(T t) const {
			detail::destroy(device(), t);
		}

		template<typename T>
		void destroy_later(T t) {
			std::unique_lock lock(_lock);
			_to_destroy.push_back({_fence, detail::VkResource(t)});
		}

	private:
		void clear_resources(u64 up_to);

		std::deque<std::pair<u64, detail::VkResource>> _to_destroy;

		concurrent::SpinLock _lock;

		std::atomic<u64> _fence = 1;
		std::atomic<u64> _done_fence = 0;
};

}

#endif // YAVE_DEVICE_RESOURCELIFETIMEMANAGER_H
