/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include <yave/device/Device.h>

#include "Queue.h"

namespace yave {

Queue::Queue(vk::Queue queue) : _queue(queue) {
}

Queue::~Queue() {
	if(_queue) {
		wait();
	}
}

Queue::Queue(Queue&& other) {
	swap(other);
}

Queue& Queue::operator=(Queue&& other) {
	swap(other);
	return *this;
}

void Queue::swap(Queue& other) {
	std::swap(_queue, other._queue);
}

vk::Queue Queue::vk_queue() const {
	return _queue;
}

void Queue::wait() const {
	_queue.waitIdle();
}

void Queue::submit_base(CmdBufferBase& base) const {
	auto cmd = base.vk_cmd_buffer();
	_queue.submit(vk::SubmitInfo()
			.setCommandBufferCount(1)
			.setPCommandBuffers(&cmd),
		base.vk_fence());
}

}

