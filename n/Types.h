/*******************************
Copyright (C) 2009-2010 gr�goire ANGERAND

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

#ifndef NTYPES_H
#define NTYPES_H

#include <cstdint>
#include <cstdlib>

namespace n
{

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef size_t uint;
typedef uint64_t uint64;
typedef uint8_t byte;

template<typename T>
class TypeInfo
{
	public:
		enum {
			isPrimitive = false,
			isPointer = false,
			isConstant = false
		};
};

template<typename T>
class TypeInfo<T *>
{
	public:
		enum {
			isPrimitive = true, // a pointer is a primitive
			isPointer = true,
			isConstant = false
		};
};

template<typename T>
class TypeInfo<T const *>
{
	public:
		enum {
			isPrimitive = true, // a pointer is a primitive
			isPointer = true,
			isConstant = true
		};
};

template<typename T>
class TypeInfo<const T>
{
	public:
		enum {
			isPrimitive = false,
			isPointer = false,
			isConstant = true
		};
};

#define N_PRIM_TYPE(type) \
template<> \
class TypeInfo<type> { public: \
	enum { isPrimitive = true, isPointer = false, isConstant = false }; \
}; \
template<> \
class TypeInfo<const type> { public: \
	enum { isPrimitive = true, isPointer = false, isConstant = true }; \
}



N_PRIM_TYPE(int);
N_PRIM_TYPE(short int);
N_PRIM_TYPE(long int);
N_PRIM_TYPE(bool);
N_PRIM_TYPE(char);
N_PRIM_TYPE(float);
N_PRIM_TYPE(double);
N_PRIM_TYPE(uint);
N_PRIM_TYPE(byte);
N_PRIM_TYPE(unsigned long int);
N_PRIM_TYPE(unsigned long long int);
N_PRIM_TYPE(unsigned short int);


} //n

#endif // NTYPES_H
