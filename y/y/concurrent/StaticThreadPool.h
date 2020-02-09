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
#include <y/core/Range.h>

#include <thread>
#include <list>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace y {
namespace concurrent {

class StaticThreadPool : NonMovable {
	private:
		using Func = core::Functor<void()>;

		struct SharedData {
			std::mutex lock;
			std::condition_variable condition;

			std::list<Func> queue;

			std::atomic<bool> run = true;
		};

	public:
		StaticThreadPool(usize thread_count = std::max(4u, std::thread::hardware_concurrency()));

		~StaticThreadPool();

		usize concurency() const;

		void schedule(Func&& func);

		void process_until_empty();

		template<typename It, typename F>
		void parallel_for_each(It begin, It end, F&& func) {
			const usize size = std::distance(begin, end);
			const usize split = concurency() * 8;
			const usize step = size / split;
			const std::unique_lock lock(_shared_data->lock);
			if(concurency()) {
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
		static void worker(std::shared_ptr<SharedData> data);


		std::shared_ptr<SharedData> _shared_data;
		core::Vector<std::thread> _threads;
};

}
}

#endif // Y_CONCURRENT_STATICTHREADPOOL_H
