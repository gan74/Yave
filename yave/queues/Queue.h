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
#ifndef YAVE_QUEUES_QUEUE_H
#define YAVE_QUEUES_QUEUE_H

#include <yave/vk/vk.h>

#include "submit.h"

namespace yave {

class Queue : NonCopyable {

	public:
		Queue(vk::Queue queue);
		~Queue();

		Queue(Queue&& other);
		Queue& operator=(Queue&& other);

		vk::Queue vk_queue() const;

		void wait() const;

		template<typename Policy, CmdBufferUsage Usage>
		auto submit(RecordedCmdBuffer<Usage>&& cmd, const Policy& policy = Policy()) const {
			static_assert(Usage != CmdBufferUsage::Secondary, "Secondary CmdBuffers can not be directly submitted");
			submit_base(cmd);
			return policy(cmd);
		}

	private:
		Queue() = default;

		void swap(Queue& other);

		void submit_base(CmdBufferBase& base) const;

		vk::Queue _queue;
};

}

#endif // YAVE_QUEUES_QUEUE_H
