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

#ifndef N_TYPES_H
#define N_TYPES_H

#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <typeinfo>
#include <iostream>

namespace n {

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef size_t uint;
typedef uint64_t uint64;
typedef uint8_t byte;

extern uint typeId;

// be wary of templates therefor be wary of bullshit

namespace internal {
typedef byte Yes[1];
typedef byte No[2];

template<typename T, bool P>
struct IsNonConstIterable
{
	static constexpr bool value = false;
};

template<typename T>
struct IsNonConstIterable<T, false>
{
	private:
		struct Fallback { int iterator; };
		struct Derived : T, Fallback { };

		template<class U>
		static No &test(decltype(U::iterator) *);

		template<typename U>
		static Yes &test(U *);

	public:
		static constexpr bool value = sizeof(test<Derived>(0)) == sizeof(Yes);
};



template<typename T, bool P>
struct IsConstIterable
{
	static constexpr bool value = false;
};


template<typename T>
struct IsConstIterable<T, false>
{
	private:
		struct Fallback { int const_iterator; };
		struct Derived : T, Fallback { };

		template<class U>
		static No &test(decltype(U::const_iterator) *);

		template<typename U>
		static Yes &test(U *);

	public:
		static constexpr bool value = sizeof(test<Derived>(0)) == sizeof(Yes);
};

template<typename T, bool P>
struct TypeContentInternal {};

template<typename T>
struct TypeContentInternal<T *, true>
{
	typedef T type;
};

template<typename T>
struct TypeContentInternal<T, false>
{
	typedef decltype(((T *)0)->operator*()) type;
};

}

template<typename From, typename To> // U from T
struct TypeConversion
{
	private:
		static internal::Yes &test(To);
		static internal::No &test(...);

		static From &from();

	public:
		static constexpr bool exists = sizeof(test(from())) == sizeof(internal::Yes);

};

template<typename T>
struct TypeInfo
{
	static constexpr bool isPrimitive = std::is_trivial<T>::value;
	static constexpr bool isPointer = false;
	static constexpr bool isConst = false;
	static constexpr bool isRef = false;

	static constexpr bool isNonConstIterable = internal::IsNonConstIterable<T, isPrimitive>::value;
	static constexpr bool isIterable = internal::IsConstIterable<T, isPrimitive>::value || isNonConstIterable;

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

	static constexpr bool isNonConstIterable = false;
	static constexpr bool isIterable = false;

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

	static constexpr bool isNonConstIterable = false;
	static constexpr bool isIterable = internal::IsConstIterable<T, isPrimitive>::value;

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

	static constexpr bool isNonConstIterable = TypeInfo<T>::isNonConstIterable;
	static constexpr bool isIterable = TypeInfo<T>::isIterable;

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

template<typename T>
struct TypeContent
{
	typedef typename internal::TypeContentInternal<T, TypeInfo<T>::isPrimitive>::type type;
};


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
