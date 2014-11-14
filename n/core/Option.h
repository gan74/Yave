/*******************************
Copyright (C) 2009-2010 grégoire ANGERAND

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

namespace n {
namespace core {

template<typename T>
class Option
{
	public:
		Option() : hasVal(false) {
		}

		Option(const T &t) : Option() {
			set(t);
		}

		Option(const Option &o) : Option() {
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

		Option &operator=(const Option &o) {
			if(o) {
				set(o);
			}
			return *this;
		}

	private:
		union
		{
			T val;
		};
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
