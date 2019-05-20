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
#include <y/core/Functor.h>
#include <y/utils/recmacros.h>

#include "archives.h"

namespace y {
namespace serde2 {

#define y_serde2_unfold_arg(N)									\
	if(!_y_serde_arc(N)) { return y::core::Err(); }


#define y_serialize2(...)																			\
	template<typename Arc, typename = std::enable_if_t<y::serde2::is_writable_archive_v<Arc>>>		\
	y::serde2::Result serialize(Arc& _y_serde_arc) const noexcept {									\
		try {																						\
			Y_REC_MACRO(Y_MACRO_MAP(y_serde2_unfold_arg, __VA_ARGS__))								\
		} catch(...) {																				\
			return y::core::Err();																	\
		}																							\
		return y::core::Ok();																		\
	}

#define y_deserialize2(...)																			\
	template<typename Arc, typename = std::enable_if_t<y::serde2::is_readable_archive_v<Arc>>>		\
	y::serde2::Result deserialize(Arc& _y_serde_arc) noexcept {										\
		try {																						\
			Y_REC_MACRO(Y_MACRO_MAP(y_serde2_unfold_arg, __VA_ARGS__))								\
		} catch(...) {																				\
			return y::core::Err();																	\
		}																							\
		return y::core::Ok();																		\
	}


#define y_serde2(...)											\
	y_serialize2(__VA_ARGS__)									\
	y_deserialize2(__VA_ARGS__)


namespace detail {
template<typename... Args>
class Checker {
	public:
		Checker(Args&&... args) : _t(std::make_tuple(y_fwd(args)...)) {
		}

		template<typename Arc>
		Result deserialize(Arc& ar) {
			decltype(_t) t;
			if(ar(t) && t == _t) {
				return core::Ok();
			}
			return core::Err();
		}

		template<typename Arc>
		Result serialize(Arc& ar) const {
			return ar(_t);
		}

	private:
		std::tuple<remove_cvref_t<Args>...> _t;
};

template<typename T>
class Function {
	public:
		Function(T&& t) : _t(y_fwd(t)) {
		}

		template<typename Arc>
		Result deserialize(Arc& ar) {
			using ret_t = typename function_traits<T>::return_type;
			using args_t = typename function_traits<T>::argument_pack ;
			constexpr bool ret_result = std::is_convertible_v<ret_t, Result>;
			if constexpr(ret_result && std::is_convertible_v<decltype(ar), args_t>) {
				return _t(ar);
			} else {
				args_t args;
				if(!ar(args)) {
					return core::Err();
				}
				if constexpr(ret_result) {
					return std::apply(_t, std::move(args));
				} else {
					std::apply(_t, std::move(args));
					return core::Ok();
				}
			}
		}

		template<typename Arc>
		Result serialize(Arc& ar) const {
			using ret_t = typename function_traits<T>::return_type;
			using args_t = typename function_traits<T>::argument_pack;
			constexpr bool ret_result = std::is_convertible_v<ret_t, Result>;
			if constexpr(ret_result && std::is_convertible_v<decltype(ar), args_t>) {
				return _t(ar);
			} else {
				if constexpr(ret_result) {
					return _t();
				} else if constexpr(std::is_void_v<ret_t>) {
					_t();
					return core::Ok();
				} else {
					return ar(_t());
				}
			}
		}


	private:
		T _t;
};

template<typename T>
class Condition : Function<T> {
	public:
		Condition(bool c, T&& t) : Function<T>(y_fwd(t)), _c(c) {
		}

		template<typename Arc>
		Result deserialize(Arc& ar) {
			if(_c) {
				return Function<T>::deserialize(ar);
			}
			return core::Ok();
		}

		template<typename Arc>
		Result serialize(Arc& ar) const {
			if(_c) {
				return Function<T>::serialize(ar);
			}
			return core::Ok();
		}


	private:
		bool _c;
};

template<typename T>
class Array {
	public:
		Array(usize s, T* t) : _s(s), _t(y_fwd(t)) {
		}

		template<typename Arc>
		Result deserialize(Arc& ar) {
			return ar.array(_t, _s);
		}

		template<typename Arc>
		Result serialize(Arc& ar) const {
			return ar.array(_t, _s);
		}


	private:
		usize _s;
		T* _t;
};

}

template<typename... Args>
detail::Checker<Args...> check(Args&&... args) {
	return detail::Checker<Args...>(y_fwd(args)...);
}

template<typename T>
detail::Function<T> func(T&& t) {
	return detail::Function<T>(y_fwd(t));
}

template<typename T>
detail::Condition<T> cond(bool c, T&& t) {
	return detail::Condition<T>(c, y_fwd(t));
}

template<typename T>
detail::Array<T> array(usize s, T* t) {
	return detail::Array<T>(s, t);
}




// -------------------------------------- tests --------------------------------------
namespace {
struct Serial {
	u32 i;
	y_serde2(i)
};
static_assert(has_serialize_v<WritableArchive, Serial>);
static_assert(has_serialize_v<WritableArchive, Serial&>);
static_assert(has_serialize_v<WritableArchive, const Serial&>);

static_assert(has_deserialize_v<ReadableArchive, Serial>);
static_assert(has_deserialize_v<ReadableArchive, Serial&>);
static_assert(has_deserialize_v<ReadableArchive, Serial&&>);
}


}
}

#endif // Y_SERDE2_SERDE_H
