/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef Y_CONCURRENT_STATICTHREADPOOL_H
#define Y_CONCURRENT_STATICTHREADPOOL_H

#include <y/utils.h>
#include <y/core/Functor.h>
#include <y/core/Vector.h>

#include <thread>
#include <deque>
#include <mutex>
#include <atomic>
#include <future>
#include <condition_variable>

namespace y {
namespace concurrent {

class StaticThreadPool : NonMovable {
	private:
		using Func = core::Functor<void()>;

		struct SharedData {
			mutable std::mutex lock;
			std::condition_variable condition;

			std::deque<Func> queue;

			std::atomic<bool> run = true;
		};

	public:
		StaticThreadPool(usize thread_count = std::max(4u, std::thread::hardware_concurrency()));
		~StaticThreadPool();

		usize concurency() const;
		usize pending_tasks() const;
		void process_until_empty();

		void schedule(Func&& func);

		template<typename F, typename R = decltype(std::declval<F>()())>
		std::future<R> schedule_with_future(F&& func) {
			struct { mutable std::promise<R> promise; } box;
			auto future = box.promise.get_future();
			schedule([b = std::move(box), f = y_fwd(func)]() { b.promise.set_value(f()); });
			return future;
		}

	private:
		void worker();

		SharedData _shared_data;
		core::Vector<std::thread> _threads;
};

class WorkerThread : public StaticThreadPool {
	public:
		WorkerThread() : StaticThreadPool(1) {
		}
};

}
}

#endif // Y_CONCURRENT_STATICTHREADPOOL_H
