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
#ifndef Y_CONCURRENT_WORKGROUP_H
#define Y_CONCURRENT_WORKGROUP_H

#include <y/core/Vector.h>
#include <y/core/Functor.h>
#include <y/core/Result.h>

#include <condition_variable>
#include <thread>
#include <future>
#include <mutex>
#include <deque>


namespace y {
namespace concurrent {

namespace detail {

template<typename T, typename F, typename = std::enable_if_t<!std::is_void<T>::value>>
inline void set_promise(std::promise<T>& p, F& f) {
	try {
		p.set_value(f());
	} catch(...) {
		p.set_exception(std::current_exception());
	}
}

template<typename T, typename F, typename = std::enable_if_t<std::is_void<T>::value>>
inline void set_promise(std::promise<void>& p, F& f) {
	try {
		f();
		p.set_value();
	} catch(...) {
		p.set_exception(std::current_exception());
	}
}

}

class WorkGroup : NonCopyable {

	using LockType = std::mutex;
	using FuncType = core::Function<void()>;

	public:
		WorkGroup(usize threads = std::thread::hardware_concurrency());
		~WorkGroup();

		void subscribe();
		bool process_one();



		template<typename F>
		auto schedule(F&& t) {
			using Ret = decltype(t());
			std::promise<Ret> promise;
			std::future<Ret> future = promise.get_future();
			{
				std::lock_guard<LockType> _(_lock);
				_tasks.emplace_back([p = std::move(promise), f = std::move(t)]() mutable { detail::set_promise<Ret>(p, f); });
			}
			_notifier.notify_one();
			return std::move(future);
		}

		template<template<typename...> typename Container = core::Vector, typename It, typename F>
		auto schedule_range(It beg, It end, F&& func) {
			Container<std::future<void>> res;
			usize conc = _threads.size();
			usize min_range = 16;
			if(conc < 2) {
				res << schedule([=, f = std::forward<F>(func)]() { std::for_each(beg, end, f); });
			} else {
				usize size = std::distance(beg, end);
				usize split = std::max(min_range, size / conc);

				for(usize i = 0; i + split < size; i += split) {
					auto next = beg;
					std::advance(next, split);
					res << schedule([=, f = std::forward<F>(func)]() { std::for_each(beg, next, f); });
					beg = next;
				}
				if(beg != end) {
					res << schedule([=, f = std::forward<F>(func)]() { std::for_each(beg, end, f); });
				}
			}
			return std::move(res);
		}

	private:
		core::Result<FuncType> take();
		core::Result<FuncType> try_take();
		void stop();

		LockType _lock;
		std::condition_variable _notifier;

		std::deque<FuncType> _tasks;

		core::Vector<std::thread> _threads;

		bool _running = true;
};


}
}

#endif // Y_CONCURRENT_WORKGROUP_H
