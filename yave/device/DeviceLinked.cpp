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

#include "DeviceLinked.h"
#include "ThreadLocalDevice.h"

namespace yave {

DevicePtr DeviceLinked::device() const {
	return _device;
}

DeviceLinked::DeviceLinked() : _device(nullptr) {
	// for putting breakpoints
}

DeviceLinked::DeviceLinked(DevicePtr dev) : _device(dev) {
	if(!dev) {
		y_fatal("Null device.");
	}
}

DeviceLinked::DeviceLinked(ThreadDevicePtr dev) : DeviceLinked(dev->device()) {
}

DeviceLinked::DeviceLinked(DeviceLinked&& other) {
	swap(other);
}

DeviceLinked& DeviceLinked::operator=(DeviceLinked&& other) {
	swap(other);
	return *this;
}

void DeviceLinked::swap(DeviceLinked& other) {
	std::swap(_device, other._device);
}





ThreadDevicePtr ThreadDeviceLinked::thread_device() const {
	return _device;
}

DevicePtr ThreadDeviceLinked::device() const {
	return _device ? _device->device() : nullptr;
}

ThreadDeviceLinked::ThreadDeviceLinked() : _device(nullptr) {
	// for putting breakpoints
}

ThreadDeviceLinked::ThreadDeviceLinked(ThreadDevicePtr dev) : _device(dev) {
	if(!dev) {
		y_fatal("Null device.");
	}
}

ThreadDeviceLinked::ThreadDeviceLinked(ThreadDeviceLinked&& other) {
	swap(other);
}

ThreadDeviceLinked& ThreadDeviceLinked::operator=(ThreadDeviceLinked&& other) {
	swap(other);
	return *this;
}

void ThreadDeviceLinked::swap(ThreadDeviceLinked& other) {
	std::swap(_device, other._device);
}


}
