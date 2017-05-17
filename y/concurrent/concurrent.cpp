#include "concurrent.h"

 #include <y/core/Vector.h>

namespace y {
namespace concurrent {

bool run_workers = true;

std::mutex task_mutex;
std::condition_variable task_condition;
concurrent::Arc<detail::ParallelTask> scheduled_task;

std::mutex workers_mutex;
core::Vector<std::thread> workers;
usize concurency_level = 1;



namespace detail {

void schedule_task(Arc<ParallelTask> task) {
	{
		std::unique_lock<std::mutex> lock(task_mutex);
		scheduled_task = task;
		task_condition.notify_all();
	}

	if(task) {
		task->run();
		task->wait();
	}
}

}






static void worker() {
	void* last = nullptr;
	while(run_workers) {
		std::unique_lock<std::mutex> lock(task_mutex);
		auto task = scheduled_task;

		if(task && task.as_ptr() != last) {
			lock.unlock();
			task->run();
			last = task.as_ptr();
		} else {
			task = nullptr;
			task_condition.wait(lock);
		}
	}
}


static void join_thread_pool() {
	std::unique_lock<std::mutex> lock(workers_mutex);
	std::for_each(workers.begin(), workers.end(), [](auto& t) { t.join(); });
	workers.clear();
}


void close_thread_pool() {
	{
		std::unique_lock<std::mutex> lock(task_mutex);
		scheduled_task = nullptr;
		run_workers = false;
		task_condition.notify_all();
	}

	join_thread_pool();
}

void init_thread_pool() {
	std::unique_lock<std::mutex> lock(workers_mutex);
	if(workers.is_empty()) {
		concurency_level = std::max(4u, std::thread::hardware_concurrency());
		for(usize i = 0; i != concurency_level - 1; ++i) {
			workers.push_back(std::thread(worker));
		}
	}
}

usize concurency() {
	return concurency_level;
}


usize probable_chunk_count() {
	return concurency_level * 2;
}

}
}
