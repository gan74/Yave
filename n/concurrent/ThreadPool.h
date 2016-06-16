/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_CONCURRENT_THREADPOOL_H
#define N_CONCURRENT_THREADPOOL_H

#include "Thread.h"
#include "SpinLock.h"
#include "SynchronizedQueue.h"
#include <n/core/Functor.h>
#include <n/core/Array.h>
#include "Future.h"
#include <n/defines.h>

namespace n {
namespace concurrent {

class DefaultThreadNumberPolicy
{
	public:
		DefaultThreadNumberPolicy() : max(1024), min(2) {
		}

		void setMaxThreadCount(uint m) {
			max = m;
			if(max) {
				min = std::min(min, max);
			}
			updateThreadCount();
		}

		void setMinThreadCount(uint m) {
			min = m;
			if(max) {
				max = std::max(min, max);
			}
			updateThreadCount();
		}

	protected:
		virtual void updateThreadCount() = 0;

		uint desiredThreadCount(uint threads, uint pendingTasks) {
			uint d = ((pendingTasks >> 1) + (threads >> 1)) >> 1;
			return std::max(min, max && max < d ? max : d);
		}

		bool shouldAjust(uint threads, uint desired) {
			if(threads < desired) {
				return (threads < min) || (threads + (threads >> 2) < desired);
			} else {
				return (max && threads > max) || (threads - (threads >> 2) > desired);
			}
		}

	private:
		uint max;
		uint min;
};

class FixedThreadNumberPolicy
{
	public:
		FixedThreadNumberPolicy(uint n) : fixed(n) {
		}

		uint desiredThreadCount(uint, uint) {
			return fixed;
		}

		bool shouldAjust(uint, uint) {
			return true;
		}

	private:
		uint fixed;
};

template<typename ThreadNumberPolicy = DefaultThreadNumberPolicy>
class ThreadPool : public ThreadNumberPolicy, NonCopyable
{
	class WorkerThread : public Thread
	{
		public:
			WorkerThread(ThreadPool<ThreadNumberPolicy> *p) : canceled(false), pool(p) {
				deleteLater();
			}

			void cancel() {
				canceled = true;
			}

		protected:
			void run() override {
				while(!canceled) {
					core::Functor<void()> func = pool->dequeue();
					func();
					pool->updateThreadCount();
				}
			}

		private:
			bool canceled;
			ThreadPool<ThreadNumberPolicy> *pool;
	};


	public:
		template<typename... Args>
		ThreadPool(Args... args) : ThreadNumberPolicy(args...) {
			updateThreadCount();
		}

		~ThreadPool() {
			for(WorkerThread *t : threads) {
				t->cancel();
			}
		}

		template<typename T, typename R = typename std::result_of<T()>::type>
		SharedFuture<R> submit(const T &func) {
			Promise<R> *promise = new Promise<R>();
			enqueue(promise, func, BoolToType<std::is_void<R>::value>());
			updateThreadCount();
			return promise->getFuture();
		}

		uint getThreadCount() const {
			threadMutex.lock();
			uint s = threads.size();
			threadMutex.unlock();
			return s;
		}

		uint getPendingTaskCount() const {
			return queue.size();
		}

		template<typename T, typename R = typename std::result_of<T()>::type>
		SharedFuture<R> operator()(const T &func) {
			return submit(func);
		}

	private:
		void updateThreadCount() {
			threadMutex.lock();
			uint desired = std::max(uint(1), this->desiredThreadCount(threads.size(), queue.size()));
			if(desired != threads.size()) {
				while(threads.size() != desired && this->shouldAjust(threads.size(), desired)) {
					if(threads.size() < desired) {
						if(!addOne()) {
							if(threads.isEmpty()) {
								fatal("Unable to start thread, thread-pool needs at least one thread.");
							}
							break;
						}
					} else {
						removeOne();
					}
				}
			}
			threadMutex.unlock();
		}

		template<typename T, typename R>
		void enqueue(Promise<R> *promise, const T &func, TrueType) {
			queue.queue(core::Functor<void()>([=]() -> void {
				func();
				promise->success();
				delete promise;
			}));
		}

		template<typename T, typename R>
		void enqueue(Promise<R> *promise, const T &func, FalseType) {
			queue.queue(core::Functor<void()>([=]() -> void {
				promise->success(func());
				delete promise;
			}));
		}

		core::Functor<void()> dequeue() {
			return queue.dequeue();
		}

		void removeOne() {
			if(!threads.isEmpty()) {
				WorkerThread *t = threads.last();
				t->cancel();
				threads.pop();
			}
		}

		bool addOne() {
			WorkerThread *th = new WorkerThread(this);
			if(th->start()) {
				threads.append(th);
				return true;
			}
			delete th;
			return false;
		}

		SynchronizedQueue<core::Functor<void()>> queue;

		mutable SpinLock threadMutex;
		core::Array<WorkerThread *> threads;
};

using DefaultThreadPool = ThreadPool<>;

}
}



#endif // THREADPOOL_H
