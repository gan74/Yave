#ifndef Y_CONCURRENT_CONCURRENT_H
#define Y_CONCURRENT_CONCURRENT_H

#include "Arc.h"
#include <y/core/Functor.h>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <y/core/String.h>

namespace y {
namespace concurrent {

namespace detail {

class ParallelTask : NonCopyable {
	public:
		ParallelTask(u32 n) : _i(0), _p(0), _n(n) {
		}

		virtual ~ParallelTask() {
		}

		virtual void run() = 0;

		void wait() {
			std::unique_lock<std::mutex> lock(_mutex);
			_cond.wait(lock, [&]() { return _p == _n; });
		}

	protected:
		bool register_done() {
			u32 p = ++_p;
			if(p == _n) {
				_cond.notify_one();
			}
			return p >= _n;
		}

		std::atomic<u32> _i;
		std::atomic<u32> _p;
		const u32 _n;

	private:
		std::mutex _mutex;
		std::condition_variable _cond;
};


void schedule_task(Arc<ParallelTask> task);

template<typename F>
void schedule_n(F&& func, u32 n) {
	struct Task : ParallelTask {
		Task(F&& f, u32 n) : ParallelTask(n), _func(f) {
		}

		void run() override {
			do {
				u32 i = _i++;
				if(i >= _n) {
					return;
				}
				_func(usize(i));
			} while(!register_done());
		}

		F _func;
	};

	schedule_task(Arc<ParallelTask>(new Task(std::forward<F>(func), n)));
}

}

void close_thread_pool();
void init_thread_pool();
usize concurency();




template<typename It, typename Func>
void parallel_block_for(It begin, It end, Func&& func) {
	using is_random_access = std::is_same<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag>;
	static_assert(is_random_access::value, "parallel_block_for only works for random access iterators");

	usize size = end - begin;
	usize chunk = std::max(usize(1), size / (concurency() * 2));

	if(!size) {
		return;
	}

	usize chunk_count = size / chunk;
	chunk_count += chunk_count * chunk != size;

	detail::schedule_n([=, &func](usize i) {
		usize first = i * chunk;
		usize last = std::min(size, first + chunk);
		func(range(begin + first, begin + last));
	}, chunk_count);
}

template<typename It, typename Func>
void parallel_for(It begin, It end, Func&& func) {
	return parallel_block_for(begin, end, [&](auto&& range) {
		for(auto&& e : range) {
			func(e);
		}
	});
}


}
}

#endif // Y_CONCURRENT_CONCURRENT_H
