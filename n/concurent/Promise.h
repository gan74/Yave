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

#ifndef N_CONCURENT_PROMISE_H
#define N_CONCURENT_PROMISE_H

#include "Future.h"


namespace n {
namespace concurent {

template<typename T>
class Promise
{
	public:
		Promise() {
		}

		void success(const T &e) {
			future.complete(e);
		}

		void fail() {
			future.fail();
		}

		SharedFuture<T> getFuture() const {
			return future;
		}

		static Promise<T> succeded(const T &e) {
			Promise<T> p;
			p.success(e);
			return p;
		}

		static Promise<T> failed() {
			Promise<T> p;
			p.fail();
			return p;
		}

	private:
		SharedFuture<T> future;
};

}
}

#endif // N_CONCURENT_PROMISE_H
