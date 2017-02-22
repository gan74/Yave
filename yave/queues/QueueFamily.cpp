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

#include "QueueFamily.h"
#include <yave/device/Device.h>


namespace yave {

QueueFamily::QueueFamily(u32 index, const vk::QueueFamilyProperties& props) : _index(index), _queue_count(props.queueCount), _flags(props.queueFlags) {
}

u32 QueueFamily::index() const {
	return _index;
}

u32 QueueFamily::count() const {
	return _queue_count;
}

vk::QueueFlags QueueFamily::flags() const {
	return _flags;
}

core::Vector<vk::Queue> QueueFamily::fetch_queues(DevicePtr dptr) const {
	auto queues = core::vector_with_capacity<vk::Queue>(_queue_count);
	for(u32 i = 0; i != _queue_count; i++) {
		queues << dptr->vk_device().getQueue(_index, i);
	}
	return queues;
}

}
