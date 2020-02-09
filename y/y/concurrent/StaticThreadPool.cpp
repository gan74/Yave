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

#include <y/core/Chrono.h>

namespace y {
namespace concurrent {

StaticThreadPool::StaticThreadPool(usize thread_count) : _shared_data(std::make_shared<SharedData>()) {
	for(usize i = 0; i != thread_count; ++i) {
		_threads.emplace_back(worker, _shared_data);
	}
}

StaticThreadPool::~StaticThreadPool() {
	_shared_data->run = false;
	_shared_data->condition.notify_all();
	for(auto& thread : _threads) {
		thread.join();
	}
}

usize StaticThreadPool::concurency() const {
	return _threads.size();
}

void StaticThreadPool::schedule(Func&& func) {
	{
		std::unique_lock lock(_shared_data->lock);
		_shared_data->queue.emplace_back(std::move(func));
	}
	_shared_data->condition.notify_one();
}

void StaticThreadPool::process_until_empty() {
	while(true) {
		std::unique_lock lock(_shared_data->lock);
		if(_shared_data->queue.empty()) {
			return;
		}
		const Func f = std::move(_shared_data->queue.front());
		_shared_data->queue.pop_front();
		lock.unlock();

		f();
	}
}

void StaticThreadPool::worker(std::shared_ptr<SharedData> data) {
	while(data->run) {
		std::unique_lock lock(data->lock);
		data->condition.wait(lock, [&] { return !data->queue.empty() || !data->run; });
		if(!data->queue.empty()) {
			const Func f = std::move(data->queue.front());
			data->queue.pop_front();
			lock.unlock();

			f();
		}
	}
}

}
}
