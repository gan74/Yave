
#include <y/core/Chrono.h>
#include <y/core/Lazy.h>
#include <y/concurrent/Arc.h>

#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <numeric>


#include <thread>
#include <mutex>
#include <condition_variable>

using namespace y;
using namespace y::core;

template<typename Ret, typename... Args>
using AFunctor = core::detail::Functor<core::Arc, Ret, Args...>;

static thread_local bool thread_run;

static std::atomic<u32> pool_counter(0);
static std::mutex pool_mutex;
static std::condition_variable pool_cond;

static AFunctor<void()> task = [](){};

template<typename F>
static auto sched_task(F&& func) {
	{
		std::unique_lock<std::mutex> lock(pool_mutex);
		task = func;
		++pool_counter;
	}
	pool_cond.notify_all();
	return func();
}

void done() {
	sched_task([]{ thread_run = false; });
}

void worker_thread() {
	thread_run = true;
	u32 last = 0;
	while(thread_run) {
		{
			u32 c = pool_counter;
			if(c != last) {
				last = c;
				task();
				continue;
			}
		}
		{
			std::unique_lock<std::mutex> lock(pool_mutex);
			if(pool_counter != last) {
				continue;
			}
			pool_cond.wait(lock);
		}
	}
}

template<typename It, typename F>
void parallel_for(It begin, It end, F&& func) {
	u32 size = std::distance(begin, end);
	u32 chunk_size = size / (std::thread::hardware_concurrency() * 2);
	chunk_size = chunk_size ? chunk_size : 1;

	std::atomic<u32> processed(0);
	std::atomic<u32> beg_index(0);

	std::mutex mutex;
	std::condition_variable done;

	bool early_exit = sched_task([&, chunk_size, size]() {
		while(true) {
			u32 start_index = beg_index.fetch_add(chunk_size);
			//log_msg(str(start_index));
			if(start_index >= size) {
				return true;
			}
			u32 end_index = std::min(size, start_index + chunk_size);

			It end = begin + end_index;
			for(auto it = begin + start_index; it != end; ++it) {
				func(*it);
			}
			if(processed.fetch_add(chunk_size) >= size - chunk_size) {
				done.notify_one();
				return false;
			}
		}
	});

	if(early_exit) {
		std::unique_lock<std::mutex> lock(mutex);
		done.wait(lock, [&processed, size](){ return processed >= size; });
	}
}

void slow() {
	for(volatile int i = 0; i != 1000000; ++i) {
	}
}


int main() {
#ifdef Y_BUILD_TESTS
	return 0;
#endif
	core::Vector<std::thread> workers;
	for(usize i = 0; i != 7; ++i) {
		workers << std::thread(worker_thread);
	}

	core::Vector<int> v(1000u, 4);
	std::iota(v.begin(), v.end(), 1);

	std::atomic<usize> sum(0);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	{
		DebugTimer timer("parallel for");
		parallel_for(v.begin(), v.end(), [&sum](usize i) { slow(); sum += i % 17; });
	}

	std::cout << sum << std::endl;
	done();
	std::for_each(workers.begin(), workers.end(), [](auto&& t) { t.join(); });

	return 0;
}




