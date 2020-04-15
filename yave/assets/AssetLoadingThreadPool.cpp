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
#include "AssetLoader.h"

#include <y/concurrent/concurrent.h>
#include <y/utils/perf.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

AssetLoadingThreadPool::LoadingJob::~LoadingJob() {
}

AssetLoadingThreadPool::LoadingJob::LoadingJob(AssetLoader* loader) : _ctx(loader) {
}

const AssetDependencies& AssetLoadingThreadPool::LoadingJob::dependencies() const {
	return _ctx.dependencies();
}

AssetLoader* AssetLoadingThreadPool::LoadingJob::parent() const {
	return _ctx.parent();
}

AssetLoadingContext& AssetLoadingThreadPool::LoadingJob::loading_context() {
	return _ctx;
}



AssetLoadingThreadPool::FunctorLoadingJob::FunctorLoadingJob(GenericAssetPtr asset, AssetLoader* loader, ReadFunc func) : LoadingJob(loader), _read(std::move(func)), _asset(std::move(asset)) {
}

void AssetLoadingThreadPool::FunctorLoadingJob::read() {
	_create = _read(loading_context());
}

void AssetLoadingThreadPool::FunctorLoadingJob::finalize(DevicePtr dptr) {
	unused(dptr);
	_create();
}

void AssetLoadingThreadPool::FunctorLoadingJob::set_dependencies_failed() {
	_asset._data->set_failed(AssetLoadingErrorType::FailedDependedy);
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

DevicePtr AssetLoadingThreadPool::device() const {
	y_debug_assert(_parent);
	return _parent->device();
}

void AssetLoadingThreadPool::wait_until_loaded(const GenericAssetPtr& ptr) {
	while(!ptr.is_loaded()) {
		process_one(y_profile_unique_lock(_lock));
	}
}

void AssetLoadingThreadPool::add_loading_job(GenericAssetPtr asset, ReadFunc func) {
	auto job = std::make_unique<FunctorLoadingJob>(std::move(asset), _parent, std::move(func));
	add_loading_job(std::move(job));
}

void AssetLoadingThreadPool::add_loading_job(std::unique_ptr<LoadingJob> job) {
	{
		const auto lock = y_profile_unique_lock(_lock);
		_loading_jobs.emplace_back(std::move(job));
	}
	_condition.notify_one();
}

void AssetLoadingThreadPool::process_one(std::unique_lock<std::mutex> lock) {
	y_debug_assert(lock.owns_lock());

	for(auto it = _finalize_jobs.begin(); it != _finalize_jobs.end(); ++it) {
		const AssetLoadingState state = (*it)->dependencies().state();
		if(state != AssetLoadingState::NotLoaded) {
			auto job = std::move(*it);
			_finalize_jobs.erase(it);
			lock.unlock();

			if(state == AssetLoadingState::Loaded) {
				job->finalize(device());
			} else if(state == AssetLoadingState::Failed) {
				job->set_dependencies_failed();
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
			job->read();
			const AssetLoadingState state = job->dependencies().state();
			if(state != AssetLoadingState::NotLoaded) {
				if(state == AssetLoadingState::Loaded) {
					job->finalize(device());
				} else if(state == AssetLoadingState::Failed) {
					job->set_dependencies_failed();
				}
				_condition.notify_all();
			} else {
				auto inner_lock = y_profile_unique_lock(_lock);
				_finalize_jobs.emplace_back(std::move(job));
			}
		}
	}
}

void AssetLoadingThreadPool::worker() {
	while(_run) {
		auto lock = y_profile_unique_lock(_lock);
		_condition.wait(lock, [this] {
			return !_loading_jobs.empty() || !_finalize_jobs.empty() ||  !_run;
		});
		process_one(std::move(lock));
	}
}

}
