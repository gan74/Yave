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
#ifndef YAVE_PHYSICALDEVICE_H
#define YAVE_PHYSICALDEVICE_H

#include "Instance.h"

namespace yave {

class PhysicalDevice : NonCopyable {
	public:
		PhysicalDevice(Instance& instance);
		~PhysicalDevice();

		vk::PhysicalDevice get_vk_physical_device() const;
		vk::PhysicalDeviceMemoryProperties get_vk_memory_properties() const;

	private:
		NotOwned<Instance&> _instance;
		vk::PhysicalDevice _device;
		vk::PhysicalDeviceMemoryProperties _memory_properties;
};

}

#endif // YAVE_PHYSICALDEVICE_H
