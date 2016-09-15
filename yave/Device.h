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
#ifndef YAVE_DEVICE_H
#define YAVE_DEVICE_H

#include "yave.h"
#include "PhysicalDevice.h"

#include "CmdBufferPool.h"

#include <yave/vk/destroy.h>

namespace yave {

class CmdBufferState;

enum QueueFamily {
	Graphics,
	Max
};

class Device : NonCopyable {

	public:


	public:
		Device(Instance& instance, const LowLevelGraphics* llg);
		~Device();

		const PhysicalDevice& get_physical_device() const;

		vk::Device get_vk_device() const;

		vk::Queue get_vk_queue(usize i) const;
		i32 get_queue_family_index(usize i) const;

		bool has_wsi_support(vk::SurfaceKHR surface) const;

		Y_TODO(remove hacky pointer)
		const LowLevelGraphics* ll_remove_me;


		template<typename T>
		void destroy(T t) const {
			if(t != T(VK_NULL_HANDLE)) {
				detail::destroy(this, t);
			}
		}


		core::Rc<CmdBufferState> create_disposable_command_buffer() const;

	private:
		void compute_queue_families();
		void create_device();

		bool are_families_complete() const;

		NotOwned<Instance&> _instance;
		PhysicalDevice _physical;

		core::Vector<const char*> _extentions;

		std::array<i32, QueueFamily::Max> _queue_familiy_indices;
		std::array<vk::Queue, QueueFamily::Max> _queues;

		vk::Device _device;
		mutable CmdBufferPool _cmd_pool;
};


template<typename T>
void DeviceLinked::destroy(T t) {
	if(_device) {
		_device->destroy(t);
	}
}



}


#endif // YAVE_DEVICE_H
