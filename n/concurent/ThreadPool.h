/*******************************
Copyright (C) 2013-2014 gr�goire ANGERAND

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

#ifndef N_CONCURENT_THREADPOOL_H
#define N_CONCURENT_THREADPOOL_H

#include "Thread.h"
#include "SynchronizedQueue.h"
#include <n/core/Functor.h>
#include <n/core/Array.h>
#include "Future.h"
#include <n/defines.h>

namespace n {
namespace concurent {

class DefaultThreadNumberPolicy
{
	public:
		DefaultThreadNumberPolicy() : max(0), min(1) {
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

template<typename ThreadNumberPolicy = DefaultThreadNumberPolicy>
class ThreadPool : public ThreadNumberPolicy
{
	class WorkerThread : public Thread
	{
		public:
			WorkerThread(SynchronizedQueue<core::Functor<void>> *q) : canceled(false), queue(q) {
				deleteLater();
			}

			void cancel() {
				canceled = true;
			}

		protected:
			void run() override {
				while(!canceled) {
					core::Functor<void> func = queue->dequeue();
					func();
				}
			}

		private:
			bool canceled;
			SynchronizedQueue<core::Functor<void>> *queue;
	};


	public:
		ThreadPool() : ThreadNumberPolicy() {
		}

		~ThreadPool() {
			for(WorkerThread *t : threads) {
				t->cancel();
			}
		}


		template<typename T>
		SharedFuture<typename std::result_of<T()>::type> submit(const T &func) {
			auto promise = new Promise<typename std::result_of<T()>::type>();
			queue.queue(core::Functor<void>([promise, func]() -> void {
				promise->success(func());
				delete promise;
			}));
			updateThreadCount();
			return promise->getFuture();
		}

		uint getThreadCount() {
			threadMutex.lock();
			uint s = threads.size();
			threadMutex.unlock();
			return s;
		}

	private:
		void updateThreadCount() {
			threadMutex.lock();
			if(!threads.size()) {
				addOne();
			} else {
				uint desired = std::max((uint)1, this->desiredThreadCount(threads.size(), queue.size()));
				if(desired != threads.size()) {
					while(this->shouldAjust(threads.size(), desired)) {
						if(threads.size() < desired) {
							addOne();
						} else {
							removeOne();
						}
					}
				}
			}
			threadMutex.unlock();
		}

		void removeOne() {
			if(threads.size() > 1) {
				WorkerThread *t = threads.last();
				t->cancel();
				threads.pop();
			}
		}

		void addOne() {
			WorkerThread *th = new WorkerThread(&queue);
			th->start();
			threads.append(th);
		}

		SynchronizedQueue<core::Functor<void>> queue;
		Mutex threadMutex;
		core::Array<WorkerThread *> threads;



};

using DefaultThreadPool = ThreadPool<>;

}
}



#endif // THREADPOOL_H
