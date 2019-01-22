/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef Y_SERDE2_SERDE_H
#define Y_SERDE2_SERDE_H

#include <y/core/Result.h>

namespace y {
namespace serde2 {

using Result = core::Result<void>;

namespace detail {

template<typename Arc, typename T>
static auto has_deserialize(T*) -> bool_type<std::is_same_v<Result, decltype(std::declval<T>().deserialize(std::declval<Arc&>()))>>;
template<typename Arc, typename T>
static auto has_deserialize(...) -> std::false_type;

template<typename Arc, typename T>
static auto has_serialize(T*) -> bool_type<std::is_same_v<Result, decltype(std::declval<T>().serialize(std::declval<Arc&>()))>>;
template<typename Arc, typename T>
static auto has_serialize(...) -> std::false_type;

}

template<typename Arc, typename T>
using is_deserializable = bool_type<decltype(detail::has_deserialize<Arc, T>(nullptr))::value>;
template<typename Arc, typename T>
using is_serializable = bool_type<decltype(detail::has_serialize<Arc, T>(nullptr))::value>;

#define y_serialize2(...)										\
	template<typename Arc>										\
	y::serde2::Result serialize(Arc& _y_serde_arc) const {		\
		return _y_serde_arc(__VA_ARGS__);						\
	}

#define y_deserialize2(...)										\
	template<typename Arc>										\
	y::serde2::Result deserialize(Arc& _y_serde_arc) {			\
		return _y_serde_arc(__VA_ARGS__);						\
	}

#define y_serde2(...)											\
	y_serialize2(__VA_ARGS__)									\
	y_deserialize2(__VA_ARGS__)


}
}

#endif // Y_SERDE2_SERDE_H
