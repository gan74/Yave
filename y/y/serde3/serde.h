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
#ifndef Y_SERDE3_SERDE_H
#define Y_SERDE3_SERDE_H

#include <y/utils.h>
#include <y/utils/recmacros.h>
#include <y/utils/detect.h>

#include <tuple>

namespace y {
namespace serde3 {

namespace detail {
template<typename T>
using has_serde3_t = decltype(std::declval<T>()._y_serde3_refl());
}

template<typename T>
static constexpr bool has_serde3_v = is_detected_v<detail::has_serde3_t, T>;

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


template<typename T>
struct NamedObject {
	T& object;
	std::string_view name;

	constexpr NamedObject(T& t, std::string_view n) : object(t), name(n) {
	}
};

template<typename T>
NamedObject(T&, std::string_view) -> NamedObject<T>;



#define y_serde3_create_item(object) y::serde3::NamedObject{object, #object},

#define y_serde3_refl_qual(qual, ...) constexpr auto _y_serde3_refl() qual { return std::tuple{Y_REC_MACRO(Y_MACRO_MAP(y_serde3_create_item, __VA_ARGS__))}; }


#define y_serde3(...)								\
	y_serde3_refl_qual(/* */, __VA_ARGS__)			\
	y_serde3_refl_qual(const, __VA_ARGS__)



}
}

#endif // Y_SERDE3_SERDE_H
