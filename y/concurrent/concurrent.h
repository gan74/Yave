#ifndef Y_CONCURRENT_CONCURRENT_H
#define Y_CONCURRENT_CONCURRENT_H

#include "Arc.h"
#include <y/core/Functor.h>
#include <y/core/Vector.h>

#include <thread>
#include <mutex>
#include <condition_variable>

namespace y {
namespace concurrent {

namespace detail {

class ParallelTask : NonCopyable {
	public:
		ParallelTask(u32 n) : _i(0), _p(0), _n(n) {
		}

		virtual ~ParallelTask() {
		}

		void run() {
			while(process_one());
		}

		bool process_one() {
			u32 i = _i++;
			if(i >= _n) {
				return false;
			}
			process(usize(i));
			return !register_done();
		}

		void wait() {
			std::unique_lock<std::mutex> lock(_mutex);
			_cond.wait(lock, [&]() { return _p == _n; });
		}

	protected:
		virtual void process(usize) = 0;

	private:
		bool register_done() {
			u32 p = ++_p;
			if(p == _n) {
				std::unique_lock<std::mutex> lock(_mutex);
				_cond.notify_one();
			}
			return p >= _n;
		}

		std::atomic<u32> _i;
		std::atomic<u32> _p;
		const u32 _n;

		std::mutex _mutex;
		std::condition_variable _cond;
};


void schedule_task(Arc<ParallelTask> task);

template<typename F>
void schedule_n(F&& func, u32 n) {
	struct Task : ParallelTask {
		Task(F&& f, u32 n) : ParallelTask(n), _func(f) {
		}

		void process(usize i) override {
			_func(i);
		}

		F _func;
	};

	schedule_task(Arc<ParallelTask>(new Task(std::forward<F>(func), n)));
}



}

void init_thread_pool();
usize concurency();
usize probable_block_count();




template<typename It, typename Func>
void parallel_indexed_block_for(It begin, It end, Func&& func) {
	usize size = end - begin;
	usize chunk = std::max(usize(1), size / (probable_block_count() - 1));

	if(!size) {
		return;
	}

	usize chunk_count = size / chunk;
	chunk_count += chunk_count * chunk != size;

	detail::schedule_n([=, &func](usize i) {
		usize first = i * chunk;
		usize last = std::min(size, first + chunk);
		using I = decltype(begin);
		func(i, range(I(begin + first), I(begin + last)));
	}, chunk_count);
}



template<typename It, typename Func>
void parallel_block_for(It begin, It end, Func&& func) {
	parallel_indexed_block_for(begin, end, [&](usize, auto&& range) {
		func(range);
	});
}

template<typename It, typename Func>
void parallel_for_each(It begin, It end, Func&& func) {
	return parallel_indexed_block_for(begin, end, [&](usize, auto&& range) {
		for(auto&& e : range) {
			func(e);
		}
	});
}

template<typename It, typename Func>
void parallel_for(It begin, It end, Func&& func) {
	return parallel_indexed_block_for(begin, end, [&](usize, auto&& range) {
		for(auto i = range.begin(); i != range.end(); ++i) {
			func(i);
		}
	});
}



template<template<typename...> typename C = core::Vector, typename It, typename Func>
auto parallel_block_collect(It begin, It end, Func&& func) {
	using T = decltype(func(std::declval<Range<It>>()));
	std::mutex mutex;

	C<T> col;
	try_reserve(col, probable_block_count());

	parallel_indexed_block_for(begin, end, [&](usize, auto&& range) {
		auto e = func(range);

		std::unique_lock<std::mutex> lock(mutex);
		col.push_back(std::move(e));
	});

	return col;
}

template<template<typename...> typename C = core::Vector, typename It, typename Func>
auto parallel_collect(It begin, It end, Func&& func) {
	auto cols = parallel_block_collect(begin, end, func);

	usize size = 0;
	for(const auto& c : cols) {
		size += c.size();
	}

	C<std::remove_reference_t<decltype(*cols.first().begin())>> sum;
	try_reserve(sum, size);

	for(const auto& c : cols) {
		std::copy(c.begin(), c.end(), std::back_inserter(sum));
	}

	return sum;
}



}
}

#endif // Y_CONCURRENT_CONCURRENT_H
