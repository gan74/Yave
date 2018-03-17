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

#include "StaticThreadPool.h"

#include <y/core/Chrono.h>

namespace y {
namespace concurrent {

StaticThreadPool::StaticThreadPool(usize thread_count) : _shared_data(new SharedData()), _thread_count(thread_count) {
	for(usize i = 0; i != _thread_count; ++i) {
		std::thread t(worker, _shared_data);
		t.detach();
	}
}

StaticThreadPool::~StaticThreadPool() {
	std::unique_lock guard(_shared_data->lock);
	_shared_data->run = false;
	_shared_data->condition.notify_all();
}

void StaticThreadPool::schedule(Func&& func) {
	std::unique_lock guard(_shared_data->lock);
	_shared_data->queue.push_back(std::move(func));
	_shared_data->condition.notify_all();
}

void StaticThreadPool::process_until_empty() {
	while(true) {
		std::unique_lock guard(_shared_data->lock);
		if(_shared_data->queue.empty()) {
			return;
		}
		Func f = std::move(_shared_data->queue.front());
		_shared_data->queue.pop_front();
		guard.unlock();

		f();
	}
}

void StaticThreadPool::worker(Arc<SharedData> data) {
	while(data->run) {
		std::unique_lock guard(data->lock);
		data->condition.wait(guard, [&] { return !data->queue.empty(); });
		Func f = std::move(data->queue.front());
		data->queue.pop_front();
		guard.unlock();

		f();
	}
}

}
}
