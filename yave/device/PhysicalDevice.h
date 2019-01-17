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
#ifndef YAVE_DEVICE_PHYSICALDEVICE_H
#define YAVE_DEVICE_PHYSICALDEVICE_H

#include "Instance.h"

namespace yave {

class PhysicalDevice : NonCopyable {
	public:
		PhysicalDevice(Instance& instance);
		~PhysicalDevice();

		vk::PhysicalDevice vk_physical_device() const;
		const vk::PhysicalDeviceProperties& vk_properties() const;
		const vk::PhysicalDeviceMemoryProperties& vk_memory_properties() const;

	private:
		Instance& _instance;
		vk::PhysicalDevice _device;
		vk::PhysicalDeviceProperties _properties;
		vk::PhysicalDeviceMemoryProperties _memory_properties;
};

}

#endif // YAVE_DEVICE_PHYSICALDEVICE_H
