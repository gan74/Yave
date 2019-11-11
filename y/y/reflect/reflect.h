/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef Y_REFLECT_REFLECT_H
#define Y_REFLECT_REFLECT_H

#include <y/utils/types.h>
#include <y/utils/recmacros.h>
#include <y/utils/detect.h>
#include <y/utils/name.h>

#include <y/core/Span.h>

#include <typeinfo>
#include <string_view>

namespace y {
namespace reflect {

#define y_reflect_init()																			\
	inline constexpr bool _y_has_reflection() const { return true; }								\
	constexpr y::reflect::RuntimeData reflection_data() const {										\
		using self_type = remove_cvref_t<decltype(*this)>;											\
		static_assert(!std::is_polymorphic_v<self_type>,											\
			"y_reflect_polymorphic should be used for polymorphic types");							\
		return y::reflect::reflection_data<self_type>();											\
	}

#define y_reflect_init_polymorphic()																\
	inline constexpr bool _y_has_reflection() const { return true; }								\
	inline constexpr bool _y_has_polymorphic_reflection() const { return true; }					\
	Y_CONSTEXPR_VIRTUAL y::reflect::RuntimeData reflection_data() const {							\
		using self_type = remove_cvref_t<decltype(*this)>;											\
		static_assert(std::is_polymorphic_v<self_type>,												\
			"y_reflect should be used for non polymorphic types");									\
		return y::reflect::reflection_data<self_type>();											\
	}



#define y_reflect_unfold_member(member)																\
	y::reflect::MemberData {																		\
		#member,																					\
		y::reflect::type<decltype(member)>(),														\
		[](const void* obj) -> const void* {														\
			return &static_cast<const SelfType*>(obj)->member;										\
		}																							\
	},

#define y_reflect_member_data(...)																	\
	template<typename SelfType>																		\
	static inline constexpr auto _y_reflection_member_data = std::array{							\
			Y_REC_MACRO(Y_MACRO_MAP(y_reflect_unfold_member, __VA_ARGS__))							\
		};



#define y_reflect_unfold_base(tpe)																	\
	y::reflect::MemberData {																		\
		#tpe,																						\
		y::reflect::type<y::reflect::detail::or_void_t<tpe>>(),										\
		[](const void* obj) -> const void* {														\
			return static_cast<const y::reflect::detail::or_void_t<tpe>*>(							\
					static_cast<const SelfType*>(obj)												\
				);																					\
		}																							\
	},

#define y_reflect_inheritance_data(...)																\
	[[maybe_unused]]																				\
	inline constexpr bool _y_has_inheritance_reflection() const { return true; }					\
	template<typename SelfType>																		\
	static inline constexpr auto _y_reflection_inheritance_data = std::array {						\
			Y_REC_MACRO(Y_MACRO_MAP(y_reflect_unfold_base, __VA_ARGS__))							\
		};




#define y_reflect_base(...)									\
	y_reflect_inheritance_data(__VA_ARGS__)


#define y_reflect(...)										\
	y_reflect_init()										\
	y_reflect_member_data(__VA_ARGS__)

#define y_reflect_polymorphic(...)							\
	y_reflect_init_polymorphic()							\
	y_reflect_member_data(__VA_ARGS__)



#define y_reflect_external(Type, ...)															\
	namespace ::y::reflect::detail {															\
		template<>																				\
		struct ReflectionMemberData<Type, false> {												\
			using SelfType = Type;																\
			static constexpr std::array member_data = {											\
				Y_REC_MACRO(Y_MACRO_MAP(y_reflect_unfold_member, __VA_ARGS__))					\
			};																					\
		};																						\
	}


namespace detail {

template<typename T = void>
using or_void_t = T;

template<typename T>
using has_reflection_t = decltype(std::declval<T>()._y_has_reflection());
template<typename T>
using has_polymorphic_reflection_t = decltype(std::declval<T>()._y_has_polymorphic_reflection());
template<typename T>
using has_inheritance_reflection_t = decltype(std::declval<T>()._y_has_inheritance_reflection());
}


template<typename T>
static constexpr bool has_reflection_v = is_detected_v<detail::has_reflection_t, T>;

template<typename T>
static constexpr bool has_polymorphic_reflection_v = is_detected_v<detail::has_polymorphic_reflection_t, T>;

template<typename T>
static constexpr bool has_inheritance_reflection_v = is_detected_v<detail::has_inheritance_reflection_t, T>;



struct MemberData;
struct Type;
struct RuntimeData;

using obj_relection_data_fn = RuntimeData (*)(const void*);
using relection_data_fn = RuntimeData (*)();

struct Type {
	const std::string_view name;
	const u32 size;
	const relection_data_fn reflection_data_func;
	const obj_relection_data_fn obj_reflection_data_func;

	const struct Flags {
		u32 is_polymorphic				: 1;
		u32 is_trivial					: 1;
		u32 is_trivially_copyable		: 1;
		u32 is_primitive				: 1;
		u32 is_void						: 1;
		u32 is_pointer					: 1;

		u32 has_reflection				: 1;
		u32 has_polymorphic_reflection	: 1;
		u32 has_inheritance_reflection	: 1;
	} flags;

	template<typename T>
	constexpr RuntimeData reflection_data(const T* t) const;
	constexpr RuntimeData reflection_data() const;

	constexpr bool operator==(const Type& other) const {
		return name == other.name;
	}

	constexpr bool operator!=(const Type& other) const {
		return !operator==(other);
	}
};

struct RuntimeData {
	const Type type;
	const core::Span<MemberData> members;
	const core::Span<MemberData> parents;
};

template<typename T>
constexpr RuntimeData Type::reflection_data(const T* t) const {
	return obj_reflection_data_func(t);
}

constexpr RuntimeData Type::reflection_data() const {
	return reflection_data_func();
}



template<typename T>
constexpr RuntimeData reflection_data();

template<typename T>
constexpr RuntimeData reflection_data(T&& t);


template<typename T>
constexpr const Type type() {
	u32 size = 0;
	using naked_t = remove_cvref_t<T>;
	if constexpr(!std::is_void_v<naked_t>) {
		size = sizeof(T);
	}
	return {
		ct_type_name<T>(),
		size,
		&reflection_data<T>,
		[](const void* p) {
			if constexpr(std::is_void_v<naked_t>) {
				unused(p);
				return reflection_data<void>();
			} else {
				return reflection_data(*static_cast<const naked_t*>(p));
			}
		},
		{
			std::is_polymorphic_v<T>,
			std::is_trivial_v<T>,
			std::is_trivially_copyable_v<T>,
			std::is_fundamental_v<T>,
			std::is_void_v<T>,
			std::is_pointer_v<T>,

			has_reflection_v<T>,
			has_polymorphic_reflection_v<T>,
			has_inheritance_reflection_v<T>,
		}
	};
}


struct MemberData {
	using get_t = const void* (*)(const void* obj);

	const std::string_view name;
	const Type type;
	const get_t get_c;

	void* get_raw(void* p) const {
		return const_cast<void*>(get_c(p));
	}

	const void* get_raw(const void* p) const {
		return get_c(p);
	}

	template<typename T>
	T& get(void* p) const {
		y_debug_assert(reflect::type<T>() == type);
		return *static_cast<T*>(get_raw(p));
	}

	template<typename T>
	const T& get(const void* p) const {
		y_debug_assert(reflect::type<T>() == type);
		return *static_cast<const T*>(get_raw(p));
	}
};



namespace detail {
template<typename T, bool B>
struct ReflectionMemberData {};
template<typename T>
struct ReflectionMemberData<T, false> {
	static constexpr std::array<MemberData, 0>  member_data = {};
};
template<typename T>
struct ReflectionMemberData<T, true> {
	static constexpr auto member_data = T::template _y_reflection_member_data<T>;
};

template<typename T, bool B>
struct ReflectionInheritanceData {};
template<typename T>
struct ReflectionInheritanceData<T, false> {
	static constexpr std::array<MemberData, 0>  inheritance_data = {};
};
template<typename T>
struct ReflectionInheritanceData<T, true> {
	static constexpr auto inheritance_data = T::template _y_reflection_inheritance_data<T>;
};
}

template<typename T>
constexpr RuntimeData reflection_data() {
	using without_ref = std::remove_reference_t<T>;
	using without_ptr = std::remove_pointer_t<without_ref>;
	return RuntimeData {
		type<without_ref>(),
		detail::ReflectionMemberData<without_ptr, has_reflection_v<without_ptr>>::member_data,
		detail::ReflectionInheritanceData<without_ptr, has_inheritance_reflection_v<without_ptr>>::inheritance_data
	};
}

template<typename T>
constexpr RuntimeData reflection_data(T&& t) {
	if constexpr(has_reflection_v<T>) {
		return t.reflection_data();
	} else {
		return reflection_data<T>();
	}
}



namespace {
struct S { int x; y_reflect(x) };
struct T {};
struct P : S { virtual ~P() {} int y; y_reflect_base(S) y_reflect_polymorphic(y) };
static_assert(has_reflection_v<S>);
static_assert(!has_inheritance_reflection_v<S>);
static_assert(!has_reflection_v<T>);
static_assert(has_reflection_v<P>);
static_assert(has_inheritance_reflection_v<P>);
static_assert(has_polymorphic_reflection_v<P>);
static_assert(!has_polymorphic_reflection_v<S>);

static_assert(type<int>() != type<float>());
static_assert(type<std::string_view>() == type<std::string_view>());
}
}
}

#endif // Y_REFLECT_REFLECT_H
