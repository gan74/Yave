/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef Y_CORE_RESULT_H
#define Y_CORE_RESULT_H

#include <y/utils.h>

namespace y {
namespace detail {

template<typename T>
struct Ok : NonCopyable {

	Ok(T&& t) : _value(std::forward<T>(t)) {
	}

	Ok(Ok&& o) : _value(std::move(o._value)) {
	}

	T _value;
};

template<typename T>
struct Err : NonCopyable {

	Err(T&& t) : _err(std::forward<T>(t)) {
	}

	Err(Err&& e) : _err(std::move(e._err)) {
	}

	T _err;
};

}

template<typename T>
auto Ok(T&& t) {
	return detail::Ok<T>(std::forward<T>(t));
}

template<typename T>
auto Err(T&& e) {
	return detail::Err<T>(std::forward<T>(e));
}


template<typename T, typename E>
class Result : NonCopyable {

	//static constexpr bool Symetric = std::is_same<T, E>::value;

	enum State {
		eError,
		eOk
	};

	public:
		template<typename U>
		Result(detail::Ok<U>&& v) : _state(eOk), _value(std::move(v._value)) {
		}

		template<typename U>
		Result(detail::Err<U>&& e) : _state(eError), _error(std::move(e._err)) {
		}

		Result(Result&& other) : _state(other._state) {
			if(is_ok()) {
				_value = std::move(other._value);
			} else {
				_error = std::move(other._error);
			}
		}


		~Result() {
			if(is_ok()) {
				_value.~T();
			} else {
				_error.~E();
			}
		}


		bool is_error() const {
			return !is_ok();
		}

		bool is_ok() const {
			return _state == eOk;
		}

		const T& unwrap() const {
			return expected("Unwrap failed");
		}

		T& unwrap() {
			return expected("Unwrap failed");
		}

		const T& unwrap_or(const T& def) const {
			return is_ok() ? _value : def;
		}


		const E& error() const {
			if(is_ok()) {
				fatal("Result is not an error");
			}
			return _error;
		}

		E& error() {
			if(is_ok()) {
				fatal("Result is not an error");
			}
			return _error;
		}


		const T& expected(const char* err_msg) const {
			if(is_error()) {
				fatal(err_msg);
			}
			return _value;
		}

		T& expected(const char* err_msg) {
			if(is_error()) {
				fatal(err_msg);
			}
			return _value;
		}

		template<typename F>
		Result<typename std::result_of<F(T)>::type, E> map(const F& f) const {
			if(is_ok()) {
				return Ok(f(_value));
			}
			return Err(_error);
		}

		template<typename F>
		Result<T, typename std::result_of<F(E)>::type> map_err(const F& f) const {
			if(is_ok()) {
				return Ok(_value);
			}
			return Err(f(_error));
		}


	private:
		State _state;

		union {
			T _value;
			E _error;
		};
};

}

#endif // Y_CORE_RESULT_H
