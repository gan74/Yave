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
#include <mutex>
#include <deque>

#include <queue>

namespace y {
namespace concurrent {

class WorkGroup : NonCopyable {

	using LockType = std::mutex;

	public:
		WorkGroup(usize threads = std::thread::hardware_concurrency());
		~WorkGroup();

		void schedule(core::Function<void()>&& t);

		void subscribe();

		bool process_one();
		void process_all();

		void operator()();


		template<typename It, typename F>
		void schedule_range(It beg, It end, F&& func) {
			usize conc = _threads.size();
			usize min_range = 16;

			if(conc < 2) {
				return schedule([=, f = std::forward<F>(func)]() {
						std::for_each(beg, end, f);
					});
			}
			{
				usize size = std::distance(beg, end);
				usize split = std::max(min_range, size / conc);

				std::lock_guard<LockType> _(_lock);
				for(usize i = 0; i + split < size; i += split) {
					auto next = beg;
					std::advance(next, split);
					_tasks.push_back([=, f = std::forward<F>(func)]() {
							std::for_each(beg, next, f);
						});
					beg = next;
				}
				if(beg != end) {
					_tasks.push_back([=, f = std::forward<F>(func)]() {
							std::for_each(beg, end, f);
						});
				}
			}
			_notifier.notify_all();
		}

	private:
		core::Result<core::Function<void()>> take();
		core::Result<core::Function<void()>> try_take();
		void stop();

		LockType _lock;
		std::condition_variable _notifier;
		std::deque<core::Function<void()>> _tasks;

		core::Vector<std::thread> _threads;

		bool _running = true;
};


}
}

#endif // Y_CONCURRENT_WORKGROUP_H
