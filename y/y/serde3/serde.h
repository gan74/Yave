/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef Y_SERDE3_SERDE_H
#define Y_SERDE3_SERDE_H

#include <y/utils.h>
#include <y/utils/recmacros.h>
#include <y/utils/detect.h>

#include <tuple>
#include <string_view>

namespace y {
namespace serde3 {

class WritableArchive;
class ReadableArchive;

namespace detail {

// https://stackoverflow.com/questions/36296425/how-to-determine-programmatically-if-an-expression-is-rvalue-or-lvalue-in-c/36312021#36312021
template<typename T>
constexpr std::is_lvalue_reference<T&&> is_lvalue(T&&);

template<typename T>
using has_serde3_t = decltype(std::declval<T>()._y_serde3_refl());
template<typename T>
using has_serde3_poly_t = decltype(std::declval<T>()->_y_serde3_poly_base);
template<typename T, typename... Args>
using has_serde3_post_deser_t = decltype(std::declval<T>().post_deserialize(std::declval<Args>()...));
template<typename T, typename... Args>
using has_serde3_poly_post_deser_t = decltype(std::declval<T>()->post_deserialize_poly(std::declval<Args>()...));
}

template<typename T>
static constexpr bool has_serde3_v = is_detected_v<detail::has_serde3_t, T>;

template<typename T>
static constexpr bool has_serde3_poly_v = is_detected_v<detail::has_serde3_poly_t, T>;

template<typename T, typename... Args>
static constexpr bool has_serde3_post_deser_v = is_detected_v<detail::has_serde3_post_deser_t, T, Args...>;

template<typename T, typename... Args>
static constexpr bool has_serde3_post_deser_poly_v = is_detected_v<detail::has_serde3_poly_post_deser_t, T, Args...>;




template<typename T>
constexpr auto members(T&& t) {
	if constexpr(has_serde3_v<T>) {
		return t._y_serde3_refl();
	} else {
		return std::tuple<>{};
	}
}

template<typename T>
constexpr usize member_count() {
	return std::tuple_size_v<decltype(members(std::declval<T&>()))>;
}

// Ref version
template<typename T, bool Ref = true>
struct NamedObject {
	T& object;
	const std::string_view name;

	constexpr NamedObject(T& t, std::string_view n) : object(t), name(n) {
	}

	constexpr NamedObject<const T> make_const_ref() const {
		return NamedObject<const T>(object, name);
	}

	constexpr NamedObject<T> make_ref() {
		return *this;
	}
};

template<typename T>
struct NamedObject<T, false> {
	T object;
	const std::string_view name;

	constexpr NamedObject(T t, std::string_view n) : object(std::move(t)), name(n) {
	}

	constexpr NamedObject<const T> make_const_ref() const {
		return NamedObject<const T>(object, name);
	}

	constexpr NamedObject<T> make_ref() {
		return NamedObject<T>(object, name);
	}
};

template<bool Ref, typename T>
constexpr auto create_named_object(T&& t, std::string_view name) {
	return NamedObject<std::remove_reference_t<T>, Ref>(y_fwd(t), name);
}

#define y_serde3_create_item(object) y::serde3::create_named_object<decltype(y::serde3::detail::is_lvalue(object))::value>(object, #object),

#define y_serde3_refl_qual(qual, ...) /*constexpr*/ auto _y_serde3_refl() qual { return std::tuple{Y_REC_MACRO(Y_MACRO_MAP(y_serde3_create_item, __VA_ARGS__))}; }


#define y_serde3(...)								\
	y_serde3_refl_qual(/* */, __VA_ARGS__)			\
	y_serde3_refl_qual(const, __VA_ARGS__)



}
}

#endif // Y_SERDE3_SERDE_H
