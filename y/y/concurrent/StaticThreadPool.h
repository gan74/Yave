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
#ifndef Y_CONCURRENT_STATICTHREADPOOL_H
#define Y_CONCURRENT_STATICTHREADPOOL_H

#include <y/utils.h>
#include <y/core/Functor.h>
#include <y/core/Vector.h>
#include <y/core/Range.h>

#include "Arc.h"

#include <thread>
#include <list>
#include <mutex>
#include <condition_variable>

namespace y {
namespace concurrent {

class StaticThreadPool : NonCopyable {
	public:
		using Func = core::Function<void()>;

	private:
		struct SharedData {
			std::mutex lock;
			std::condition_variable condition;

			std::list<Func> queue;

			bool run = true;
		};

	public:
		StaticThreadPool(usize thread_count = std::max(4u, std::thread::hardware_concurrency()));

		~StaticThreadPool();

		void schedule(Func&& func);

		void process_until_empty();

		template<typename It, typename F>
		void parallel_for_each(It begin, It end, F&& func) {
			usize size = std::distance(begin, end);
			usize split = _thread_count * 8;
			usize step = size / split;
			std::unique_lock lock(_shared_data->lock);
			if(_thread_count) {
				It next = begin + step;
				for(usize i = 0; i != split - 1; ++i) {
					_shared_data->queue.push_back([=] {
						for(auto&& e : core::Range(begin, next)) {
							func(e);
						}
					});
					begin = next;
					next = begin + step;
				}
			}
			_shared_data->queue.push_back([=] {
				for(auto&& e : core::Range(begin, end)) {
					func(e);
				}
			});
			_shared_data->condition.notify_all();
		}

	private:
		static void worker(Arc<SharedData> data);

		Arc<SharedData> _shared_data;
		usize _thread_count;
};

}
}

#endif // Y_CONCURRENT_STATICTHREADPOOL_H
