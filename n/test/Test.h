/*******************************
Copyright (C) 2013-2014 grégoire ANGERAND

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

#ifndef N_TEST_TEST_H
#define N_TEST_TEST_H

#include <n/defines.h>
#include <functional>
#include <cstring>

namespace n {
namespace test {
	class TestFailedException : public std::exception
	{
		public:
			TestFailedException(const char *c) : std::exception(), w(new char[strlen(c) + 1]) {
				strcpy(w, c);
			}

			~TestFailedException() {
				delete[] w;
			}

			const char *what() const noexcept override {
				return w;
			}

		private:
			char *w;
	};


	template<typename T>
	bool test(T &&a, const T &b, const char *msg) {
		if(a != b) {
			throw TestFailedException(msg);
		}
		return true;
	}

	template<typename T>
	bool test(const T &a, const T &b, const char *msg) {
		if(a != b) {
			throw TestFailedException(msg);
		}
		return true;
	}

	template<typename T>
	bool test(T f, const typename std::result_of<T()>::type &r, const char *msg) {
		return test(f(), r, msg);
	}


} //test
} //n

#endif // N_TEST_TEST_H
