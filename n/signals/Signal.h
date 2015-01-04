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

#ifndef N_SIGNALS_SIGNAL
#define N_SIGNALS_SIGNAL

#include <n/concurent/SpinLock.h>
#include <n/concurent/Async.h>
#include <n/core/Array.h>
#include <n/core/Functor.h>

namespace n {
namespace signals {

class ImmediateCall : core::NonCopyable
{
	public:
		template<typename... Args>
		void call(const core::Array<core::Functor<void(Args...)>> &slots, Args... args) const {
			for(auto f : slots) {
				f(args...);
			}
		}
};

class AsyncCall : core::NonCopyable
{
	public:
		template<typename... Args>
		void call(const core::Array<core::Functor<void(Args...)>> &slots, Args... args) const {
			concurent::Async([=]() {
				for(auto f : slots) {
					f(args...);
				}
			});
		}
};

template<typename CallingPolicy, typename... Args>
class Signal : private CallingPolicy
{
	using Lock = concurent::SpinLock;
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

		void connect(const core::Functor<void(Args...)> &f) {
			lock.lock();
			connected.append(f);
			lock.unlock();
		}

		template<typename T, typename R>
		void connect(T *t, R (T::*f)(Args...)) {
			connect([=](Args... args) {
				(t->*f)(args...);
			});
		}

		template<typename T>
		void connect(const T &t) {
			connect(core::Functor<void(Args...)>([=] (Args... args) {
				t(args...);
			}));
		}

	private:
		mutable Lock lock;
		core::Array<core::Functor<void(Args...)>> connected;
};

template<typename... Args>
using BasicSignal = Signal<ImmediateCall, Args...>;

template<typename... Args>
using AsyncSignal = Signal<AsyncCall, Args...>;

}
}

#endif // N_SIGNALS_SIGNAL

