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

#include "WorkGroup.h"

namespace y {
namespace concurrent {


WorkGroup::WorkGroup(usize threads) {
	for(usize i = 0; i != threads; ++i) {
		_threads << std::thread([this]() mutable { subscribe(); });
	}
}

WorkGroup::~WorkGroup() {
	stop();
	subscribe();
	std::for_each(_threads.begin(), _threads.end(), [](auto& t) { t.join(); });
}

void WorkGroup::subscribe() {
	while(process_one());
}

bool WorkGroup::process_one() {
	auto work = take();
	if(work.is_error()) {
		return false;
	}
	work.unwrap()();
	return true;
}

core::Result<WorkGroup::FuncType> WorkGroup::take() {
	std::unique_lock<LockType> lock(_lock);
	while(_running || !_tasks.empty()) {
		if(_tasks.empty()) {
			_notifier.wait(lock);
		} else {
			auto task = std::move(_tasks.front());
			_tasks.pop_front();
			return core::Ok(std::move(task));
		}
	}
	return core::Err();
}

core::Result<WorkGroup::FuncType> WorkGroup::try_take() {
	std::unique_lock<LockType> _(_lock);
	if(_tasks.empty()) {
		return core::Err();
	}
	FuncType task = std::move(_tasks.front());
	_tasks.pop_front();
	return core::Ok(std::move(task));
}

void WorkGroup::stop() {
	{
		std::lock_guard<LockType> _(_lock);
		_running = false;
	}
	_notifier.notify_all();
}

}
}
