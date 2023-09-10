/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

AssetLoadingThreadPool::AssetLoadingThreadPool(AssetLoader* parent, usize concurency) : _parent(parent) {
    _threads = core::Vector<std::thread>::with_capacity(concurency);
    for(usize i = 0; i != concurency; ++i) {
        _threads.emplace_back([this] {
            concurrent::set_thread_name("Asset loading thread");
            worker();
        });
    }
}

AssetLoadingThreadPool::~AssetLoadingThreadPool() {
    {
        _run = false;
        const auto lock = std::unique_lock(_lock);
        _condition.notify_all();
    }

    for(auto& thread : _threads) {
        thread.join();
    }
}

void AssetLoadingThreadPool::wait_until_loaded(const GenericAssetPtr& ptr) {
    y_profile();
    while(ptr.is_loading()) {
        process_one(std::unique_lock(_lock));
    }
}

void AssetLoadingThreadPool::add_loading_job(std::unique_ptr<LoadingJob> job) {
    {
        const auto lock = std::unique_lock(_lock);
        _loading_jobs.emplace_back(std::move(job));
    }
    _condition.notify_one();
}

bool AssetLoadingThreadPool::is_processing() const {
    return _processing != 0;
}

void AssetLoadingThreadPool::process_one(std::unique_lock<std::mutex> lock) {
    y_profile();
    y_debug_assert(lock.owns_lock());

    ++_processing;
    y_defer(--_processing);

    {
        y_profile_zone("finalizing loop");
        for(auto it = _finalize_jobs.begin(); it != _finalize_jobs.end(); ++it) {
            const AssetLoadingState state = (*it)->dependencies().state();
            if(state != AssetLoadingState::NotLoaded) {
                auto job = std::move(*it);
                _finalize_jobs.erase(it);
                lock.unlock();

                if(state == AssetLoadingState::Loaded) {
                    job->finalize();
                } else if(state == AssetLoadingState::Failed) {
                    job->set_dependencies_failed();
                }
                _condition.notify_all();
                return;
            }
        }
    }

    y_debug_assert(lock.owns_lock());

    if(!_loading_jobs.is_empty()) {
        y_profile_zone("load one");
        auto job = std::move(_loading_jobs.first());
        _loading_jobs.pop_front();
        lock.unlock();

        if(job->read()) {
            y_profile_zone("post read");
            const AssetLoadingState state = job->dependencies().state();
            if(state != AssetLoadingState::NotLoaded) {
                if(state == AssetLoadingState::Loaded) {
                    job->finalize();
                } else if(state == AssetLoadingState::Failed) {
                    job->set_dependencies_failed();
                }
                _condition.notify_all();
            } else {
                auto inner_lock = std::unique_lock(_lock);
                _finalize_jobs.emplace_back(std::move(job));
            }
        }
    }
}

void AssetLoadingThreadPool::worker() {
    while(_run) {
        auto lock = std::unique_lock(_lock);
        _condition.wait(lock, [this] {
            return !_loading_jobs.is_empty() || !_finalize_jobs.empty() ||  !_run;
        });
        process_one(std::move(lock));
    }
}

}

