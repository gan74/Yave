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
#include <type_traits>
#include <typeinfo>

namespace n {

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef size_t uint;
typedef uint64_t uint64;
typedef uint8_t byte;

extern uint typeId;

// be wary of templates therefor be wary of bullshit

template<typename T>
struct TypeInfo
{
	static constexpr bool isPrimitive = std::is_trivial<T>::value;
	static constexpr bool isPointer = false;
	static constexpr bool isConst = false;
	static constexpr bool isRef = false;
	static const uint baseId;
	static const uint id;

	typedef T nonRef;
	typedef T nonConst;
	typedef T nonPtr;
};



template<typename T>
struct TypeInfo<T *>
{
	static constexpr bool isPrimitive = TypeInfo<T>::isPrimitive;
	static constexpr bool isPointer = true;
	static constexpr bool isConst = TypeInfo<T>::isConst;
	static constexpr bool isRef = TypeInfo<T>::isRef;
	static const uint baseId;
	static const uint id;

	typedef typename TypeInfo<T>::nonRef *nonRef;
	typedef typename TypeInfo<T>::nonConst *nonConst;
	typedef T nonPtr;
};

template<typename T>
struct TypeInfo<const T>
{
	static constexpr bool isPrimitive = TypeInfo<T>::isPrimitive;
	static constexpr bool isPointer = TypeInfo<T>::isPointer;
	static constexpr bool isConst = true;
	static constexpr bool isRef = TypeInfo<T>::isRef;
	static const uint baseId;
	static const uint id;

	typedef const typename TypeInfo<T>::nonRef nonRef;
	typedef T nonConst;
	typedef const typename TypeInfo<T>::nonPtr nonPtr;
};

template<typename T>
struct TypeInfo<T &>
{
	static constexpr bool isPrimitive = TypeInfo<T>::isPrimitive;
	static constexpr bool isPointer = TypeInfo<T>::isPointer;
	static constexpr bool isConst = TypeInfo<T>::isConst;
	static constexpr bool isRef = true;
	static const uint baseId;
	static const uint id;

	typedef T nonRef;
	typedef typename TypeInfo<T>::nonConst &nonConst;
	typedef typename TypeInfo<T>::nonPtr &nonPtr;
};


template<typename T>
const uint TypeInfo<T>::baseId = typeId++; // dependent on compilation, but NOT on execution flow
template<typename T>
const uint TypeInfo<T>::id = TypeInfo<T>::baseId;

template<typename T>
const uint TypeInfo<T *>::id = typeId++;
template<typename T>
const uint TypeInfo<T *>::baseId = TypeInfo<T>::baseId;

template<typename T>
const uint TypeInfo<const T>::id = typeId++;
template<typename T>
const uint TypeInfo<const T>::baseId = TypeInfo<T>::baseId;

template<typename T>
const uint TypeInfo<T &>::id = typeId++;
template<typename T>
const uint TypeInfo<T &>::baseId = TypeInfo<T>::baseId;



/*template<typename T>
class TypeInfo
{
	template<typename U>
	struct ptr
	{
		static const bool value = false;
	};

	template<typename U>
	struct ptr<U *>
	{
		static const bool value = true;
	};

	template<typename U>
	struct cst
	{
		static const bool value = false;
	};

	template<typename U>
	struct cst<const U>
	{
		static const bool value = true;
	};

	public:
		static const bool isPrimitive = std::is_trivial<T>::value;
		static const bool isPointer = ptr<T>::value;
		static const bool isConst = cst<T>::value;
};*/

} //n

#endif // NTYPES_H
