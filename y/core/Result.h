/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef Y_CORE_RESULT_H
#define Y_CORE_RESULT_H

#include <y/utils.h>

namespace y {
namespace core {

namespace detail {

template<typename T>
struct Ok {

	Ok(T&& t) : _value(std::move(t)) {
	}

	Ok(const T& t) : _value(t) {
	}

	const T& get() const {
		return _value;
	}

	T& get() {
		return _value;
	}

	private:
		T _value;
};

template<typename T>
struct Err {

	Err(T&& t) : _err(std::move(t)) {
	}

	Err(const T& t) : _err(t) {
	}

	const T& get() const {
		return _err;
	}

	T& get() {
		return _err;
	}

	private:
		T _err;
};

template<>
struct Ok<void> : NonCopyable {
	Ok() {
	}

	Ok(Ok&&) {
	}

	void get() const {
	}
};


template<>
struct Err<void> : NonCopyable {
	Err() {
	}

	Err(Err&&) {
	}

	void get() const {
	}
};

}


inline auto Ok() {
	return detail::Ok<void>();
}

inline auto Err() {
	return detail::Err<void>();
}

template<typename T>
inline auto Ok(T&& t) {
	return detail::Ok<std::remove_const_t<std::remove_reference_t<T>>>(std::forward<T>(t));
}

template<typename T>
inline auto Err(T&& e) {
	return detail::Err<std::remove_const_t<std::remove_reference_t<T>>>(std::forward<T>(e));
}



template<typename T, typename E = void>
class Result : NonCopyable {

	public:
		using value_type = T;
		using error_type = E;

	private:
		using ok_type = detail::Ok<value_type>;
		using err_type = detail::Err<error_type>;

		template<typename F, typename U>
		struct map_type {
			using type = decltype(std::declval<F>()(std::declval<U>()));
		};

		template<typename F>
		struct map_type<F, void> {
			using type = decltype(std::declval<F>()());
		};

		using expected_type = decltype(std::declval<ok_type>().get());
		using expected_const_type = decltype(std::declval<const ok_type>().get());

	public:

		Result(ok_type&& v) : _is_ok(true) {
			new(&_value) ok_type(std::move(v));
		}

		Result(err_type&& e) : _is_ok(false) {
			new(&_error) err_type(std::move(e));
		}

		Result(Result&& other) {
			move(other);
		}

		~Result() {
			destroy();
		}

		Result& operator=(Result&& other) {
			destroy();
			move(other);
			return *this;
		}

		bool is_error() const {
			return !is_ok();
		}

		bool is_ok() const {
			return _is_ok;
		}

		auto&& unwrap() const {
			return expected("Unwrap failed.");
		}

		auto&& unwrap() {
			return expected("Unwrap failed.");
		}

		auto&& error() const {
			if(is_ok()) {
				fatal("Result is not an error.");
			}
			return _error.get();
		}

		auto&& error() {
			if(is_ok()) {
				fatal("Result is not an error.");
			}
			return _error.get();
		}

		expected_const_type expected(const char* err_msg) const {
			if(is_error()) {
				fatal(err_msg);
			}
			return _value.get();
		}

		expected_type expected(const char* err_msg) {
			if(is_error()) {
				fatal(err_msg);
			}
			return _value.get();
		}

		template<typename U>
		std::enable_if_t<!std::is_void_v<value_type>, const U&> unwrap_or(const U& f) const {
			return is_ok() ? _value.get() : f;
		}

		template<typename U>
		std::enable_if_t<!std::is_void_v<error_type>, const U&> error_or(const U& f) const {
			return is_error() ? _error.get() : f;
		}

		template<typename F>
		Result<typename map_type<F, value_type>::type, error_type> map(const F& f) const {
			if(is_ok()) {
				if constexpr(std::is_void_v<value_type>) {
					return Ok(f());
				} else {
					return Ok(f(_value.get()));
				}
			}
			if constexpr(std::is_void_v<error_type>) {
				return Err();
			} else {
				return Err(_error.get());
			}
		}

		template<typename F>
		Result<value_type, typename map_type<F, error_type>::type> map_err(const F& f) const {
			if(is_ok()) {
				if constexpr(std::is_void_v<value_type>) {
					return Ok();
				} else {
					return Ok(_value.get());
				}
			}
			if constexpr(std::is_void_v<error_type>) {
				return Err(f());
			} else {
				return Err(f(_error.get()));
			}
		}

	protected:
		void destroy() {
			if(is_ok()) {
				_value.~ok_type();
			} else {
				_error.~err_type();
			}
		}

		void move(Result& other) {
			if((_is_ok = other.is_ok())) {
				new(&_value) ok_type(std::move(other._value));
			} else {
				new(&_error) err_type(std::move(other._error));
			}
		}

		union {
			ok_type _value;
			err_type _error;
		};
		bool _is_ok;
};



}
}

#endif // Y_CORE_RESULT_H
