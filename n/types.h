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
#ifndef N_TYPES_H
#define N_TYPES_H

#include <n/defines.h>

#include <algorithm>
#include <type_traits>
#include <typeinfo>

namespace n {

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t byte;
typedef size_t uint;









class NonCopyable
{
	public:
		NonCopyable() {}
		NonCopyable(const NonCopyable &) = delete;
		NonCopyable &operator=(const NonCopyable &) = delete;
};



namespace details {
	struct NullType
	{
		NullType() = delete;
	};
	extern uint typeId;
}

// be wary of templates therefor be wary of bullshit

template<typename O, typename...>
O &makeOne();

struct Nothing
{
	template<typename... Args>
	Nothing(Args...) {}

	template<typename... Args>
	Nothing operator()(Args...) const {
		return *this;
	}

	template<typename T>
	operator T() const {
		return fatal("Nothing used.");
	}
};

template<int I>
struct IntToType
{
	static constexpr bool value = I;
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

template<bool I, typename Then, typename Else>
struct If
{
	typedef Then type;
};


template<typename Then, typename Else>
struct If<false, Then, Else>
{
	typedef Else type;
};

#define N_GEN_TYPE_HAS_MEMBER2(className, member) \
template<typename HasMemberType> \
class className { \
	template<typename U, bool P> \
	struct SFINAE { \
		struct NoType { }; \
		struct Fallback { NoType member; }; \
		template<typename B> \
		struct Derived : B, Fallback { }; \
		template<typename T> \
		static FalseType test(decltype(Derived<T>::member) *); \
		template<typename T> \
		static BoolToType<!std::is_same<typename T::member *, T *>::value> test(typename T::member *); \
		static constexpr bool value = decltype(test<U>(0))::value; \
	}; \
	template<typename U> \
	struct SFINAE<U, true> { \
		static constexpr bool value = false; \
	}; \
	static constexpr bool isPrimitive = !std::is_class<HasMemberType>::value && !std::is_union<HasMemberType>::value; \
	public: \
		static constexpr bool value = SFINAE<HasMemberType, isPrimitive>::value; \
};

#define N_GEN_TYPE_HAS_MEMBER(className, member) N_GEN_TYPE_HAS_MEMBER2(className, member)
/*template<typename HasMemberType> \
class className { \
	typedef byte Yes[1]; \
	typedef byte No[2]; \
	template<typename U, bool B> \
	struct SFINAE { \
		struct Fallback { int member; }; \
		struct Derived : HasMemberType, Fallback { }; \
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
	static constexpr bool isPrimitive = !std::is_class<HasMemberType>::value && !std::is_union<HasMemberType>::value; \
	public: \
		static constexpr bool value = SFINAE<HasMemberType, isPrimitive>::value; \
};*/


#define N_GEN_TYPE_HAS_METHOD(className, method) \
template<typename HasMethodType, typename HasMethodRetType, typename... HasMethodArgsType> \
class className { \
	template<typename U, bool B> \
	struct SFINAE { \
		template<typename V> \
		static auto test(V *) -> BoolToType<std::is_same<decltype(makeOne<V>().method(makeOne<HasMethodArgsType>()...)), HasMethodRetType>::value>; \
		template<typename V> \
		static FalseType test(...); \
		static constexpr bool value = decltype(test<U>(0))::value; \
	}; \
	template<typename U> \
	struct SFINAE<U, true> { \
		static constexpr bool value = false; \
	}; \
	public: \
		static constexpr bool value = SFINAE<HasMethodType, std::is_fundamental<HasMethodType>::value>::value; \
};

template<typename T>
class StrongTypeHelper
{
	public:
		StrongTypeHelper(const T &e) : t(e){
		}

		const T &get() const {
			return t;
		}

	private:
		const T &t;
};

#define N_GEN_CLASS_OP(cl, op) \
template<typename T \
/*, typename X = typename std::enable_if<!n::TypeConversion<T, cl>::exists>::type*/> \
decltype(n::makeOne<cl>() op n::makeOne<cl>()) \
operator op(const T &i, const n::StrongTypeHelper<cl> &s) { return (cl(i) op s.get()); }

namespace details {
	N_GEN_TYPE_HAS_MEMBER(IsConstIterableInternal, const_iterator)
	N_GEN_TYPE_HAS_MEMBER(IsNonConstIterableInternal, iterator)

	template<typename T, bool B>
	struct IsNonConstIterableDispatch
	{
		static constexpr bool value = !std::is_same<typename T::iterator, NullType>::value;
	};

	template<typename T>
	struct IsNonConstIterableDispatch<T, false>
	{
		static constexpr bool value = false;
	};

	template<typename T>
	struct IsNonConstIterable
	{
		static constexpr bool value = IsNonConstIterableDispatch<T, IsNonConstIterableInternal<T>::value>::value;
	};

	template<typename T, bool B>
	struct IsConstIterableDispatch
	{
		static constexpr bool value = !std::is_same<typename T::const_iterator, NullType>::value;
	};

	template<typename T>
	struct IsConstIterableDispatch<T, false>
	{
		static constexpr bool value = false;
	};

	template<typename T>
	struct IsConstIterable
	{
		static constexpr bool value = IsConstIterableDispatch<T, IsConstIterableInternal<T>::value>::value;
	};


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
		typedef decltype((reinterpret_cast<T *>(0))->operator*()) type;
	};

	template<typename T, bool P>
	struct IsDereferenceable // P = false
	{

		template<typename U>
		static TrueType test(decltype(&U::operator*));
		template<typename U>
		static FalseType test(...);

		public:
			static constexpr bool value = decltype(test<T>(0))::value;
	};

	template<typename T>
	struct IsDereferenceable<T, true>
	{
		static constexpr bool value = false;
	};

	template<typename T, bool B>
	class InheritAlready : public T
	{
	};

	template<typename T>
	class InheritAlready<T, true>
	{
	};
}

template<typename T>
struct VoidToNothing : If<!std::is_void<T>::value, T, Nothing>
{
};


template<typename T, typename Base>
class InheritIfNotAlready : public details::InheritAlready<Base, std::is_base_of<Base, T>::value>
{
};

template<typename From, typename To> // U from T
class TypeConversion
{
	struct CanBuildFrom
	{
		template<typename U>
		static TrueType build(decltype(U(makeOne<From>()))*);

		template<typename U>
		static FalseType build(...);

		static const bool value = decltype(build<To>(0))::value;
	};

	static TrueType test(To);
	static FalseType test(...);

	public:
		static constexpr bool makeArithmeticSence = !std::is_arithmetic<From>::value || !std::is_arithmetic<To>::value ||
											(std::is_floating_point<From>::value == std::is_floating_point<To>::value && sizeof(To) >= sizeof(From));
		static constexpr bool exists = decltype(test(std::move(makeOne<From>())))::value;
		static constexpr bool existsWeak = exists && makeArithmeticSence;
		static constexpr bool canBuild = exists || CanBuildFrom::value;

};

class TypeData
{
	public:
		TypeData(const std::type_info &i) : info(&i) {
		}

		template<typename T>
		explicit TypeData(const T &t) : TypeData(typeid(t)) {
		}


		bool operator==(const TypeData &t) const {
			return *info == *t.info;
		}

		bool operator!=(const TypeData &t) const {
			return !operator ==(t);
		}

		bool operator>(const TypeData &t) const {
			return info->before(*t.info);
		}

		bool operator<(const TypeData &t) const {
			return t.info->before(*info);
		}

		core::String name() const;

	private:
		const std::type_info *info;
};

#ifdef N_GCC_4
#define N_HAS_CPY_CTOR has_trivial_copy_constructor<T>
#else
#define N_HAS_CPY_CTOR is_trivially_copy_constructible<T>
#endif


template<typename T>
struct TypeInfo
{
	static constexpr bool isPrimitive = !std::is_class<T>::value && !std::is_union<T>::value;
	static constexpr bool isPod = std::N_HAS_CPY_CTOR::value || isPrimitive;
	static constexpr bool isPointer = false;
	static constexpr bool isConst = false;
	static constexpr bool isRef = false;
	static const uint baseId;
	static const uint id;

	static constexpr bool isNonConstIterable = details::IsNonConstIterable<T>::value;
	static constexpr bool isIterable = details::IsConstIterable<T>::value || isNonConstIterable;

	static constexpr bool isDereferenceable = details::IsDereferenceable<T, isPrimitive>::value;

	static const TypeData type;

	typedef T nonRef;
	typedef T nonConst;
	typedef T nonPtr;
	typedef T decayed;
};

#undef N_HAS_CPY_CTOR

template<typename T>
struct TypeInfo<T *>
{
	static constexpr bool isPod = true;
	static constexpr bool isPrimitive = true;
	static constexpr bool isPointer = true;
	static constexpr bool isConst = TypeInfo<T>::isConst;
	static constexpr bool isRef = TypeInfo<T>::isRef;

	static const uint baseId;
	static const uint id;

	static constexpr bool isNonConstIterable = false;
	static constexpr bool isIterable = false;

	static constexpr bool isDereferenceable = true;

	static const TypeData type;

	typedef typename TypeInfo<T>::nonRef *nonRef;
	typedef T* nonConst;
	typedef T nonPtr;
	typedef typename TypeInfo<T>::decayed decayed;
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
	static constexpr bool isIterable = details::IsConstIterable<T>::value;

	static constexpr bool isDereferenceable = TypeInfo<T>::isDereferenceable;

	static const TypeData type;

	typedef const typename TypeInfo<T>::nonRef nonRef;
	typedef T nonConst;
	typedef const typename TypeInfo<T>::nonPtr nonPtr;
	typedef typename TypeInfo<T>::decayed decayed;
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

	static constexpr bool isDereferenceable = TypeInfo<T>::isDereferenceable;

	static const TypeData type;

	typedef T nonRef;
	typedef T& nonConst;
	typedef typename TypeInfo<T>::nonPtr &nonPtr;
	typedef typename TypeInfo<T>::decayed decayed;
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

	static constexpr bool isDereferenceable = true;

	static const TypeData type;

	typedef T nonRef;
	typedef typename TypeInfo<T>::nonConst nonConst;
	typedef typename TypeInfo<T>::nonPtr nonPtr;
	typedef typename TypeInfo<T>::decayed decayed;
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

	static constexpr bool isDereferenceable = true;

	static const TypeData type;

	typedef T nonRef;
	typedef typename TypeInfo<T>::nonConst &nonConst;
	typedef typename TypeInfo<T>::nonPtr &nonPtr;
	typedef typename TypeInfo<T>::decayed decayed;
};

template<typename T, typename... Args>
struct IsThreadSafe
{
	static constexpr bool value = IsThreadSafe<T>::value && IsThreadSafe<Args...>::value;
};

template<typename T>
struct IsThreadSafe<T>
{
	static constexpr bool value = TypeInfo<typename TypeInfo<T>::decayed>::isPod;
};


template<typename T>
const uint TypeInfo<T>::baseId = details::typeId++; // dependent on compilation, but NOT on execution flow
template<typename T>
const uint TypeInfo<T>::id = TypeInfo<T>::baseId;
template<typename T>
const TypeData TypeInfo<T>::type = typeid(T);

template<typename T>
const uint TypeInfo<T *>::id = details::typeId++;
template<typename T>
const uint TypeInfo<T *>::baseId = TypeInfo<T>::baseId;
template<typename T>
const TypeData TypeInfo<T *>::type = typeid(T *);

template<typename T>
const uint TypeInfo<const T>::id = details::typeId++;
template<typename T>
const uint TypeInfo<const T>::baseId = TypeInfo<T>::baseId;
template<typename T>
const TypeData TypeInfo<const T>::type = typeid(const T);

template<typename T>
const uint TypeInfo<T &>::id = details::typeId++;
template<typename T>
const uint TypeInfo<T &>::baseId = TypeInfo<T>::baseId;
template<typename T>
const TypeData TypeInfo<T &>::type = typeid(T &);

template<typename T>
const uint TypeInfo<T[]>::id = details::typeId++;
template<typename T>
const uint TypeInfo<T[]>::baseId = TypeInfo<T>::baseId;
template<typename T>
const TypeData TypeInfo<T[]>::type = typeid(T[]);

template<typename T, uint N>
const uint TypeInfo<T[N]>::id = details::typeId++;
template<typename T, uint N>
const uint TypeInfo<T[N]>::baseId = TypeInfo<T>::baseId;
template<typename T, uint N>
const TypeData TypeInfo<T[N]>::type = typeid(T[N]);

template<typename T>
struct TypeContent
{
	typedef typename details::TypeContentInternal<T, TypeInfo<T>::isPrimitive || !TypeInfo<T>::isDereferenceable>::type type;
};


} //n

#endif // NTYPES_H
