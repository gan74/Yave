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
#include "concurrent.h"
#include "SpinLock.h"

#include <y/core/Vector.h>

namespace y {
namespace concurrent {

static bool run_workers = true;

static std::mutex scheduler_mutex;
static std::condition_variable scheduler_condition;
static Arc<detail::ParallelTask> scheduled_task;

static usize concurency_level = 1;


namespace detail {

ParallelTask::ParallelTask(u32 n) : _i(0), _p(0), _n(n) {
}

ParallelTask::~ParallelTask() {
}

void ParallelTask::run() {
	while(process_one());
}

bool ParallelTask::process_one() {
	u32 i = _i++;
	if(i >= _n) {
		return false;
	}
	process(usize(i));
	return !register_done();
}

void ParallelTask::wait() {
	std::unique_lock lock(_mutex);
	_cond.wait(lock, [&]() { return _p == _n; });
}

bool ParallelTask::register_done() {
	u32 p = ++_p;
	if(p == _n) {
		std::unique_lock lock(_mutex);
		_cond.notify_one();
	}
	return p >= _n;
}



void schedule_task(Arc<ParallelTask> task) {
	static SpinLock global_sched_lock;

	while(!global_sched_lock.try_lock()) {
		if(!task->process_one()) {
			return;
		}
	}

	{
		std::unique_lock lock(scheduler_mutex);
		scheduled_task = task;
	}
	scheduler_condition.notify_all();

	task->run();
	task->wait();

	global_sched_lock.unlock();
}

}




static void work() {
	void* last = nullptr;
	while(run_workers) {
		std::unique_lock lock(scheduler_mutex);
		auto task = scheduled_task;

		while(!task || task.as_ptr() == last) {
			scheduler_condition.wait(lock);
			task = scheduled_task;
		}

		lock.unlock();
		task->run();
		last = task.as_ptr();
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
	return concurency_level * 16;
}

usize probable_block_count(usize size) {
	usize loglog = log2ui(log2ui(size));
	return std::clamp(concurency_level * (loglog * loglog), usize(1), size);
}

}
}
