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

struct NullType
{
	NullType() = delete;
};


template<bool B>
struct BoolToType // false
{
	static constexpr bool value = B;
	BoolToType() {}
	BoolToType(std::false_type) {}
	operator std::false_type() const {
		return std::false_type();
	}
};

template<>
struct BoolToType<true>
{
	static constexpr bool value = true;
	BoolToType() {}
	BoolToType(std::true_type) {}
	operator std::true_type() const {
		return std::true_type();
	}
};

typedef BoolToType<false> FalseType;
typedef BoolToType<true> TrueType;

#define N_GEN_TYPE_HAS_MEMBER(className, member) \
template<typename T> \
class className { \
	typedef byte Yes[1]; \
	typedef byte No[2]; \
	template<typename U, bool B> \
	struct SFINAE { \
		struct Fallback { int member; }; \
		struct Derived : T, Fallback { }; \
		template<class V> \
		static No &test(decltype(V::member) *); \
		template<typename V> \
		static Yes &test(V *); \
		static constexpr bool value = sizeof(test<Derived>(0)) == sizeof(Yes); \
	}; \
	template<typename U> \
	struct SFINAE<U, true> { \
		static constexpr bool value = false; \
	}; \
	static constexpr bool isPrimitive = !std::is_class<T>::value && !std::is_union<T>::value; \
	public: \
		static constexpr bool value = SFINAE<T, isPrimitive>::value; \
};

#define N_GEN_TYPE_HAS_METHOD(className, method) \
template<typename T, typename R, typename... Args> \
class className { \
	template<typename U, bool B> \
	struct SFINAE { \
		template<typename V> \
		static auto test(V *) -> typename std::is_same<decltype(std::declval<V>().method(std::declval<Args>()... )), R>::type; \
		template<typename V> \
		static std::false_type test(...); \
		typedef decltype(test<U>(0)) type; \
	}; \
	template<typename U> \
	struct SFINAE<U, true> { \
		typedef std::false_type type; \
	}; \
	public: \
		static constexpr bool value = SFINAE<T, std::is_fundamental<T>::value>::type::value; \
};

namespace internal {
	N_GEN_TYPE_HAS_MEMBER(IsConstIterable, const_iterator)
	N_GEN_TYPE_HAS_MEMBER(IsNonConstIterable, iterator)

	template<typename T, bool P>
	struct TypeContentInternal // P = false
	{
		typedef NullType type;
	};

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

	template<typename T, bool P>
	struct IsDereferencable // P = false
	{
		private:
			typedef byte Yes[1];
			typedef byte No[2];
			template<typename U>
			static Yes &test(decltype(&U::operator*));
			static No &test(...);
		public:
			static constexpr bool value = sizeof(test(0)) == sizeof(Yes);
	};

	template<typename T>
	struct IsDereferencable<T, true>
	{
		static constexpr bool value = false;
	};
}

template<typename From, typename To> // U from T
class TypeConversion
{
	typedef byte Yes[1];
	typedef byte No[2];
	static Yes &test(To);
	static No &test(...);

	static From &from();
	public:
		static constexpr bool exists = sizeof(test(from())) == sizeof(Yes);

};

template<typename T>
struct TypeInfo
{
	static constexpr bool isPrimitive = std::is_fundamental<T>::value;// !std::is_class<T>::value && !std::is_union<T>::value;
	static constexpr bool isPod = std::is_trivial<T>::value || isPrimitive;
	static constexpr bool isPointer = false;
	static constexpr bool isConst = false;
	static constexpr bool isRef = false;

	static constexpr bool isNonConstIterable = internal::IsNonConstIterable<T>::value;
	static constexpr bool isIterable = internal::IsConstIterable<T>::value || isNonConstIterable;

	static constexpr bool isDereferencable = internal::IsDereferencable<T, isPod>::value;

	static const uint baseId;
	static const uint id;

	typedef T nonRef;
	typedef T nonConst;
	typedef T nonPtr;
};

template<typename T>
struct TypeInfo<T *>
{
	static constexpr bool isPod = TypeInfo<T>::isPod;
	static constexpr bool isPrimitive = true;
	static constexpr bool isPointer = true;
	static constexpr bool isConst = TypeInfo<T>::isConst;
	static constexpr bool isRef = TypeInfo<T>::isRef;
	static const uint baseId;
	static const uint id;

	static constexpr bool isNonConstIterable = false;
	static constexpr bool isIterable = false;

	static constexpr bool isDereferencable = true;

	typedef typename TypeInfo<T>::nonRef *nonRef;
	typedef typename TypeInfo<T>::nonConst *nonConst;
	typedef T nonPtr;
};

template<typename T>
struct TypeInfo<const T>
{
	static constexpr bool isPod = TypeInfo<T>::isPod;
	static constexpr bool isPrimitive = TypeInfo<T>::isPrimitive;
	static constexpr bool isPointer = TypeInfo<T>::isPointer;
	static constexpr bool isConst = true;
	static constexpr bool isRef = TypeInfo<T>::isRef;
	static const uint baseId;
	static const uint id;

	static constexpr bool isNonConstIterable = false;
	static constexpr bool isIterable = internal::IsConstIterable<T>::value;

	static constexpr bool isDereferencable = TypeInfo<T>::isDereferencable;

	typedef const typename TypeInfo<T>::nonRef nonRef;
	typedef T nonConst;
	typedef const typename TypeInfo<T>::nonPtr nonPtr;
};

template<typename T>
struct TypeInfo<T &>
{
	static constexpr bool isPod = TypeInfo<T>::isPod;
	static constexpr bool isPrimitive = TypeInfo<T>::isPrimitive;
	static constexpr bool isPointer = TypeInfo<T>::isPointer;
	static constexpr bool isConst = TypeInfo<T>::isConst;
	static constexpr bool isRef = true;
	static const uint baseId;
	static const uint id;

	static constexpr bool isNonConstIterable = TypeInfo<T>::isNonConstIterable;
	static constexpr bool isIterable = TypeInfo<T>::isIterable;

	static constexpr bool isDereferencable = TypeInfo<T>::isDereferencable;

	typedef T nonRef;
	typedef typename TypeInfo<T>::nonConst &nonConst;
	typedef typename TypeInfo<T>::nonPtr &nonPtr;
};

template<typename T>
struct TypeInfo<T[]>
{
	static constexpr bool isPod = TypeInfo<T>::isPod;
	static constexpr bool isPrimitive = TypeInfo<T>::isPrimitive;
	static constexpr bool isPointer = false;
	static constexpr bool isConst = TypeInfo<T>::isConst;
	static constexpr bool isRef = false;
	static const uint baseId;
	static const uint id;

	static constexpr bool isNonConstIterable = false;
	static constexpr bool isIterable = false;

	static constexpr bool isDereferencable = true;

	typedef T nonRef;
	typedef typename TypeInfo<T>::nonConst &nonConst;
	typedef typename TypeInfo<T>::nonPtr &nonPtr;
};

template<typename T, uint N>
struct TypeInfo<T[N]>
{
	static constexpr bool isPod = TypeInfo<T>::isPod;
	static constexpr bool isPrimitive = TypeInfo<T>::isPrimitive;
	static constexpr bool isPointer = false;
	static constexpr bool isConst = TypeInfo<T>::isConst;
	static constexpr bool isRef = false;
	static const uint baseId;
	static const uint id;

	static constexpr bool isNonConstIterable = false;
	static constexpr bool isIterable = false;

	static constexpr bool isDereferencable = true;

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
const uint TypeInfo<T[]>::id = typeId++;
template<typename T>
const uint TypeInfo<T[]>::baseId = TypeInfo<T>::baseId;

template<typename T>
struct TypeContent
{
	typedef typename internal::TypeContentInternal<T, TypeInfo<T>::isPod || !TypeInfo<T>::isDereferencable>::type type;
};


} //n

#endif // NTYPES_H
