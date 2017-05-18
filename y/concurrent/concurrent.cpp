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
