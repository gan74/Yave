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

#ifndef N_SCRIPT_PRIMITIVE_H
#define N_SCRIPT_PRIMITIVE_H

namespace n {
namespace script {

union Primitive
{
	Primitive() : Primitive(0) {
	}

	Primitive(int i) : Int(i) {
	}

	Primitive(float f) : Float(f) {
	}

	template<typename T>
	Primitive(T *p) : ptr(p) {
	}

	template<typename T>
	T &to() {
		return *(T *)ptr;
	}

	template<typename T>
	const T &to() const {
		return *(T *)ptr;
	}

	template<typename T>
	operator T() {
		return *(T *)ptr;
	}

	template<typename T>
	operator const T() const {
		return *(T *)ptr;
	}

	int Int;
	float Float;
	void *ptr;
};

}
}

#endif // N_SCRIPT_PRIMITIVE_H
