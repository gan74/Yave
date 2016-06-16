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

#ifndef N_SIGNALS_SIGNAL_H
#define N_SIGNALS_SIGNAL_H

#include <n/concurrent/SpinLock.h>
#include <n/concurrent/Async.h>
#include <n/core/Array.h>
#include <n/core/Functor.h>

namespace n {
namespace signals {

class ImmediateCall : NonCopyable
{
	public:
		template<typename... Args>
		void call(const core::Array<core::Functor<void(Args...)>> &slots, Args... args) const {
			for(const core::Functor<void(Args...)> &f : slots) {
				f(args...);
			}
		}
};

class AsyncCall : NonCopyable
{
	public:
		template<typename... Args>
		void call(const core::Array<core::Functor<void(Args...)>> &slots, Args... args) const {
			concurrent::Async([=]() {
				for(const core::Functor<void(Args...)> &f : slots) {
					f(args...);
				}
			});
		}
};

template<typename CallingPolicy, typename... Args>
class Signal : private CallingPolicy
{
	using Lock = concurrent::SpinLock;
	public:
		template<typename... PArgs>
		Signal(PArgs... args) : CallingPolicy(args...) {
		}

		template<typename P>
		Signal(const Signal<P, Args...> &s) {
			s.lock.lock();
			connected = s.connected;
			s.lock.unlock();
		}

		void operator()(Args... args) const {
			lock.lock();
			this->call(connected, args...);
			lock.unlock();
		}

		void connect(const core::Functor<void(Args...)> &f) const {
			lock.lock();
			connected.append(f);
			lock.unlock();
		}

		template<typename T, typename R>
		void connect(T *t, R (T::*f)(Args...)) const {
			connect([=](Args... args) {
				(t->*f)(args...);
			});
		}

		template<typename T>
		void connect(const T &t) const {
			connect(core::Functor<void(Args...)>([=] (Args... args) {
				t(args...);
			}));
		}

	private:
		mutable Lock lock;
		mutable core::Array<core::Functor<void(Args...)>> connected;
};

template<typename... Args>
using BasicSignal = Signal<ImmediateCall, Args...>;

template<typename... Args>
using AsyncSignal = Signal<AsyncCall, Args...>;

}
}

#endif // N_SIGNALS_SIGNAL_H

