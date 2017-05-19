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
#include "concurrent.h"
#include "SpinLock.h"

#include <y/core/Vector.h>

namespace y {
namespace concurrent {

/*struct CloseOnExit {
	~CloseOnExit() {
		close_thread_pool();
	}
} close_on_exit;*/

bool run_workers = true;

std::mutex scheduler_mutex;
std::condition_variable scheduler_condition;
Arc<detail::ParallelTask> scheduled_task;

usize concurency_level = 1;


namespace detail {

void schedule_task(Arc<ParallelTask> task) {
	static SpinLock global_sched_lock;

	while(!global_sched_lock.try_lock()) {
		if(!task->process_one()) {
			return;
		}
	}

	{
		std::unique_lock<std::mutex> lock(scheduler_mutex);
		scheduled_task = task;
		scheduler_condition.notify_all();
	}

	task->run();
	task->wait();

	global_sched_lock.unlock();
}

}




static void work() {
	void* last = nullptr;
	while(run_workers) {
		std::unique_lock<std::mutex> lock(scheduler_mutex);
		auto task = scheduled_task;

		if(task && task.as_ptr() != last) {
			lock.unlock();
			task->run();
			last = task.as_ptr();
		} else {
			task = nullptr;
			scheduler_condition.wait(lock);
		}
	}
}

void init_thread_pool() {
	static SpinLock lock;
	if(lock.try_lock()) {
		concurency_level = std::max(4u, std::thread::hardware_concurrency());
		for(usize i = 0; i != concurency_level - 1; ++i) {
			(new std::thread(work))->detach();
		}
	}
}

usize concurency() {
	return concurency_level;
}


usize probable_block_count() {
	return concurency_level * 2;
}

}
}
