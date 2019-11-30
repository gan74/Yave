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

#include <yave/device/Device.h>

#include "Queue.h"

namespace yave {

Queue::Queue(DevicePtr dptr, vk::Queue queue) :
		DeviceLinked(dptr),
		_queue(queue),
		_lock(std::make_unique<std::mutex>()){
}

Queue::~Queue() {
	if(device()) {
		wait();
	}
}


vk::Queue Queue::vk_queue() const {
	return _queue;
}

void Queue::wait() const {
	const std::unique_lock lock(*_lock);
	_queue.get().waitIdle();
}

Semaphore Queue::submit_sem(RecordedCmdBuffer&& cmd) const {
	const Semaphore sync(device());
	cmd._proxy->data()._signal = sync;
	submit_base(cmd);
	return sync;
}

void Queue::submit_base(CmdBufferBase& base) const {
	const std::unique_lock lock(*_lock);

	auto cmd = base.vk_cmd_buffer();

	const auto& wait = base._proxy->data()._waits;
	auto wait_semaphores = core::vector_with_capacity<vk::Semaphore>(wait.size());
	std::transform(wait.begin(), wait.end(), std::back_inserter(wait_semaphores), [](const auto& s) { return s.vk_semaphore(); });
	const core::Vector<vk::PipelineStageFlags> stages(wait.size(), vk::PipelineStageFlagBits::eAllCommands);

	const Semaphore& signal = base._proxy->data()._signal;
	vk::Semaphore sig_semaphore = signal.device() ? signal.vk_semaphore() : vk::Semaphore();

	_queue.get().submit(vk::SubmitInfo()
			.setSignalSemaphoreCount(signal.device() ? 1 : 0)
			.setPSignalSemaphores(signal.device() ? &sig_semaphore : nullptr)
			.setWaitSemaphoreCount(wait_semaphores.size())
			.setPWaitSemaphores(wait_semaphores.data())
			.setPWaitDstStageMask(stages.data())
			.setCommandBufferCount(1)
			.setPCommandBuffers(&cmd),
		base.vk_fence());
}

}

