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

#include "StaticThreadPool.h"
#include "concurrent.h"

#include <y/core/Chrono.h>

namespace y {
namespace concurrent {

StaticThreadPool::StaticThreadPool(usize thread_count, const char* thread_names) {
	for(usize i = 0; i != thread_count; ++i) {
		_threads.emplace_back([thread_names, this] {
			concurrent::set_thread_name(thread_names);
			worker();
		});
	}
}

StaticThreadPool::~StaticThreadPool() {
	_shared_data.run = false;
	_shared_data.condition.notify_all();
	for(auto& thread : _threads) {
		thread.join();
	}
}

usize StaticThreadPool::concurency() const {
	return _threads.size();
}

usize StaticThreadPool::pending_tasks() const {
	const std::unique_lock lock(_shared_data.lock);
	return _shared_data.queue.size();
}

void StaticThreadPool::process_until_empty() {
	while(true) {
		std::unique_lock lock(_shared_data.lock);
		if(_shared_data.queue.empty()) {
			return;
		}
		const Func f = std::move(_shared_data.queue.front());
		_shared_data.queue.pop_front();
		lock.unlock();

		f();
	}
}

void StaticThreadPool::wait_all_finished() {

	std::unique_lock lock(_shared_data.lock);
	const u64 up_to = _shared_data.done + _shared_data.queue.size();
	lock.unlock();

	while(_shared_data.done != up_to) {
		process_until_empty();
	}
}

void StaticThreadPool::schedule(Func&& func, bool in_front) {
	{
		const std::unique_lock lock(_shared_data.lock);
		if(in_front) {
			_shared_data.queue.emplace_front(std::move(func));
		} else {
			_shared_data.queue.emplace_back(std::move(func));
		}
	}

	if(concurency()) {
		_shared_data.condition.notify_one();
	} else {
		process_until_empty();
	}
}


void StaticThreadPool::worker() {
	while(_shared_data.run) {
		std::unique_lock lock(_shared_data.lock);
		_shared_data.condition.wait(lock, [&] { return !_shared_data.queue.empty() || !_shared_data.run; });
		if(!_shared_data.queue.empty()) {
			const Func f = std::move(_shared_data.queue.front());
			_shared_data.queue.pop_front();
			lock.unlock();


			f();
			++_shared_data.done;
		}
	}
}

}
}
