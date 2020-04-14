/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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

#include "AssetLoadingThreadPool.h"
#include "AssetLoadingContext.h"

#include <y/concurrent/concurrent.h>
#include <y/utils/perf.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {


AssetLoadingThreadPool::LoadingJob::LoadingJob(GenericAssetPtr a, ReadFunc f) :  asset(std::move(a)), func(std::move(f)) {
}

AssetLoadingThreadPool::CreateJob::CreateJob(GenericAssetPtr a, AssetLoadingContext ctx, CreateFunc f) : asset(std::move(a)), loading_ctx(std::move(ctx)), func(std::move(f)) {
}


AssetLoadingThreadPool::AssetLoadingThreadPool(AssetLoader* parent, usize concurency) : _parent(parent) {
	_threads = core::vector_with_capacity<std::thread>(concurency);
	for(usize i = 0; i != concurency; ++i) {
		_threads.emplace_back([this] {
			concurrent::set_thread_name("Asset loading thread");
			worker();
		});
	}
}

AssetLoadingThreadPool::~AssetLoadingThreadPool() {
	{
		const auto lock = y_profile_unique_lock(_lock);
		_run = false;
	}
	_condition.notify_all();

	for(auto& thread : _threads) {
		thread.join();
	}
}

void AssetLoadingThreadPool::wait_until_loaded(const GenericAssetPtr& ptr) {
	while(!ptr.is_loaded()) {
		process_one(y_profile_unique_lock(_lock));
	}
}

void AssetLoadingThreadPool::add_loading_job(GenericAssetPtr asset, ReadFunc func) {
	y_always_assert(!asset.is_empty(), "Invalid asset");
	{
		const auto lock = y_profile_unique_lock(_lock);
		_loading_jobs.emplace_back(std::move(asset), std::move(func));
	}
	_condition.notify_one();
}

void AssetLoadingThreadPool::process_one(std::unique_lock<std::mutex> lock) {
	y_debug_assert(lock.owns_lock());

	for(auto it = _create_jobs.begin(); it != _create_jobs.end(); ++it) {
		const AssetLoadingState state = it->loading_ctx.dependencies().state();
		if(state != AssetLoadingState::NotLoaded) {
			auto job = std::move(*it);
			_create_jobs.erase(it);
			lock.unlock();

			if(state == AssetLoadingState::Loaded) {
				job.func();
			} else if(state == AssetLoadingState::Failed) {
				job.asset._data->set_failed(AssetLoadingErrorType::FailedDependedy);
			}
			_condition.notify_all();
			return;
		}
	}

	y_debug_assert(lock.owns_lock());

	if(!_loading_jobs.empty()) {
		auto job = std::move(_loading_jobs.front());
		_loading_jobs.pop_front();
		lock.unlock();

		{
			AssetLoadingContext loading_ctx(_parent);
			auto create_func = job.func(loading_ctx);
			const AssetLoadingState state = loading_ctx.dependencies().state();
			if(state != AssetLoadingState::NotLoaded) {
				if(state == AssetLoadingState::Loaded) {
					create_func();
				} else if(state == AssetLoadingState::Failed) {
					job.asset._data->set_failed(AssetLoadingErrorType::FailedDependedy);
				}
				_condition.notify_all();
			} else {
				auto inner_lock = y_profile_unique_lock(_lock);
				_create_jobs.emplace_back(std::move(job.asset), std::move(loading_ctx), std::move(create_func));
			}
		}
	}
}

void AssetLoadingThreadPool::worker() {
	while(_run) {
		auto lock = y_profile_unique_lock(_lock);
		_condition.wait(lock, [this] {
			return !_loading_jobs.empty() || !_create_jobs.empty() ||  !_run;
		});
		process_one(std::move(lock));
	}
}

}
