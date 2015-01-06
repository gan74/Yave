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

#ifndef N_CONCURENT_FUTURE_H
#define N_CONCURENT_FUTURE_H

#include "MultiThreadPtr.h"
#include "Mutex.h"
#include <n/core/Option.h>

namespace n {
namespace concurent {

template<typename T>
class Promise;

template<typename T>
class SharedFuture
{
	typedef typename VoidToNothing<T>::type TI;
	public:
		enum State
		{
			Waiting,
			Failed,
			Succeded
		};

		bool isWaiting() const {
			return shared->state == Waiting;
		}

		bool isFailed() const {
			return shared->state == Failed;
		}

		bool isSuccess() const {
			return shared->state == Succeded;
		}

		core::Option<T> get() {
			wait();
			if(!isSuccess()) {
				return core::Option<T>();
			}
			return shared->val;
		}

		core::Option<T> tryGet() {
			if(!isSuccess()) {
				return core::Option<T>();
			}
			return shared->val;
		}

		void wait() {
			Mutex *m = getMutex();
			m->lock();
			while(isWaiting()) {
				m->wait();
			}
			m->unlock();
		}

		operator T() {
			return get();
		}

	private:
		friend class Promise<T>;

		struct Internal
		{
			Internal() : state(Waiting) {
			}

			~Internal() {
				mutex.lock();
				if(state == Succeded) {
					val.~TI();
				}
				mutex.unlock();
			}

			State state;
			Mutex mutex;
			union
			{
				TI val;
			};
		};

		SharedFuture() : shared(new Internal()){
		}

		void complete(TI e) {
			Mutex *m = getMutex();
			m->lock();
			if(!isWaiting()) {
				m->unlock();
				nError("Invalid future state");
				m->notifyAll();
			}
			new(&(shared->val)) TI(e);
			shared->state = Succeded;
			m->unlock();
			m->notifyAll();
		}

		void fail() {
			Mutex *m = getMutex();
			m->lock();
			if(!isWaiting()) {
				m->unlock();
				nError("Invalid future state");
				m->notifyAll();
			}
			shared->state = Failed;
			m->unlock();
			m->notifyAll();
		}

		Mutex *getMutex() {
			return &shared->mutex;
		}

		MultiThreadPtr<Internal> shared;
};

}
}

#endif // N_CONCURENT_FUTURE_H
