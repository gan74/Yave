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

#ifndef N_CONCURRENT_FUTURE_H
#define N_CONCURRENT_FUTURE_H

#include "MultiThreadPtr.h"
#include "Mutex.h"

namespace n {
namespace concurrent {

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
			Succeded,
			Uninitialized
		};

		SharedFuture() : shared(new Internal(Uninitialized)) {
		}

		bool isWaiting() const {
			return shared->state == Waiting;
		}

		bool isFailed() const {
			return shared->state == Failed;
		}

		bool isSuccess() const {
			return shared->state == Succeded;
		}

		bool isUninitialized() const {
			return shared->state == Uninitialized;
		}

		State getState() const {
			return shared->state;
		}

		T const *get() const {
			wait();
			if(!isSuccess()) {
				return 0;
			}
			return &shared->val;
		}

		T const *tryGet() const {
			if(!isSuccess()) {
				return 0;
			}
			return &shared->val;
		}

		const TI &get(const TI &def) const {
			wait();
			if(!isSuccess()) {
				return def;
			}
			return shared->val;
		}

		const TI &tryGet(const TI &def) const {
			if(!isSuccess()) {
				return def;
			}
			return shared->val;
		}

		const TI &unsafeGet() const {
			/*if(!isSuccess()) {
				fatal("SharedFuture<T>::unsafeGet failed.");
			}*/
			return shared->val;
		}

		void wait() const {
			Mutex *m = getMutex();
			m->lock();
			while(isWaiting()) {
				m->wait();
			}
			m->unlock();
		}

	private:
		friend class Promise<T>;

		struct Internal
		{
			Internal(State st = Waiting) : state(st) {
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

		SharedFuture(const Promise<T> &) : shared(new Internal()){
		}

		void complete(TI e) {
			Mutex *m = getMutex();
			m->lock();
			if(!isWaiting()) {
				m->unlock();
				fatal("Invalid future state");
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
				fatal("Invalid future state");
				m->notifyAll();
			}
			shared->state = Failed;
			m->unlock();
			m->notifyAll();
		}

		Mutex *getMutex() const {
			return &shared->mutex;
		}


		mutable MultiThreadPtr<Internal> shared;
};

}
}

#endif // N_CONCURRENT_FUTURE_H
