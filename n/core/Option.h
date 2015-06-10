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

#ifndef N_CORE_OPTION_H
#define N_CORE_OPTION_H

#include <n/types.h>

namespace n {
namespace core {

template<typename T>
class Option
{
	public:
		Option() : hasVal(false) {
		}

		Option(const T &t) : Option<T>() {
			set(t);
		}

		template<typename U>
		Option(const Option<U> &o) : Option<T>() {
			if(o) {
				set(o);
			}
		}

		Option(const Option<T> &o) : Option<T>() {
			if(o) {
				set(o);
			}
		}

		~Option() {
			clear();
		}

		void set(const T &t) {
			if(hasVal) {
				val = t;
			} else {
				new(&val) T(t);
				hasVal = true;
			}
		}

		void set(T &&t) {
			if(hasVal) {
				val = t;
			} else {
				new(&val) T(t);
				hasVal = true;
			}
		}

		void clear() {
			if(hasVal) {
				val.~T();
			}
			hasVal = false;
		}

		bool hasValue() const {
			return hasVal;
		}

		const T &get() const {
			return val;
		}

		const T &get(const T &def) const {
			return hasVal ? val : def;
		}

		operator bool() const {
			return hasVal;
		}

		operator T() const {
			return val;
		}

		template<typename U>
		Option<T> &operator=(const Option<U> &o) {
			if(o) {
				set(o);
			}
			return *this;
		}

		Option<T> &operator=(const Option<T> &o) {
			if(o) {
				set(o);
			}
			return *this;
		}

		template<typename U>
		Option<T> &operator=(const U &o) {
			set(o);
			return *this;
		}

	private:
		union
		{
			T val;
		};
		bool hasVal;
};

template<>
class Option<void>
{
	public:
		Option(Nothing = Nothing()) : hasVal(false) {
		}

		template<typename T>
		Option(const Option<T> &o) : hasVal(o.hasVal) {
		}

		void set(Nothing = Nothing()) {
			hasVal = true;
		}

		void clear() {
			hasVal = false;
		}

		bool hasValue() const {
			return hasVal;
		}

		void get() const {
		}

		operator bool() const {
			return hasVal;
		}

		operator void() const {
		}

		operator Nothing() const {
			return Nothing();
		}

		template<typename T>
		Option<void> &operator=(const Option<T> &o) {
			hasVal = o.hasVal;
			return *this;
		}

	private:
		bool hasVal;
};

struct None
{
	template<typename T>
	operator Option<T>() const {
		return Option<T>();
	}
};

} //core
} //n


#endif // N_CORE_OPTION_H
