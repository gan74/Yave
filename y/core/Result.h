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

	template<typename U>
	Ok(Ok<U>&& o) : _value(std::move(o._value)) {
	}

	const T& get() const {
		return _value;
	}

	T& get() {
		return _value;
	}

	T _value;
};

template<typename T>
struct Err : NonCopyable {

	Err(T&& t) : _err(std::forward<T>(t)) {
	}

	template<typename U>
	Err(Err<U>&& e) : _err(std::move(e._err)) {
	}

	const T& get() const {
		return _err;
	}

	T& get() {
		return _err;
	}

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


auto Ok() {
	return detail::Ok<void>();
}

auto Err() {
	return detail::Err<void>();
}

template<typename T>
auto Ok(T&& t) {
	return detail::Ok<T>(std::forward<T>(t));
}

template<typename T>
auto Err(T&& e) {
	return detail::Err<T>(std::forward<T>(e));
}


namespace detail {

template<typename T, typename E>
class Result : NonCopyable {

	static constexpr bool is_void = std::is_void<T>::value;
	using Err_t = detail::Err<E>;
	using Ok_t = detail::Ok<T>;

	enum State {
		eError,
		eOk
	};

	public:
		template<typename U>
		Result(detail::Ok<U>&& v) : _state(eOk) {
			new(&_value) Ok_t(std::move(v));
		}

		template<typename U>
		Result(detail::Err<U>&& e) : _state(eError) {
			new(&_error) Err_t(std::move(e));
		}

		Result(Result&& other) : _state(other._state) {
			if(is_ok()) {
				new(&_value) Ok_t(std::move(other._value));
			} else {
				new(&_error) Err_t(std::move(other._error));
			}
		}


		~Result() {
			if(is_ok()) {
				_value.~Ok_t();
			} else {
				_error.~Err_t();
			}
		}


		bool is_error() const {
			return !is_ok();
		}

		bool is_ok() const {
			return _state == eOk;
		}

		auto unwrap() const {
			return expected("Unwrap failed");
		}

		auto unwrap() {
			return expected("Unwrap failed");
		}

		auto error() const {
			if(is_ok()) {
				fatal("Result is not an error");
			}
			return _error.get();
		}

		auto error() {
			if(is_ok()) {
				fatal("Result is not an error");
			}
			return _error.get();
		}


		auto expected(const char* err_msg) const {
			if(is_error()) {
				fatal(err_msg);
			}
			return _value.get();
		}

		auto expected(const char* err_msg) {
			if(is_error()) {
				fatal(err_msg);
			}
			return _value.get();
		}



	protected:
		State _state;

		union {
			Ok_t _value;
			Err_t _error;
		};
};

}




template<typename T, typename E>
struct Result : detail::Result<T, E> {

	using detail::Result<T, E>::Result;

	auto unwrap_or(const T& f) const {
		return this->is_ok() ? this->_value.get() : f;
	}

	template<typename F>
	Result<typename std::result_of<F(T)>::type, E> map(const F& f) const {
		if(this->is_ok()) {
			return Ok(f(this->_value.get()));
		}
		return Err(this->_error.get());
	}

	template<typename F>
	Result<T, typename std::result_of<F(E)>::type> map_err(const F& f) const {
		if(this->is_ok()) {
			return Ok(this->_value.get());
		}
		return Err(f(this->_error.get()));
	}
};

template<typename E>
struct Result<void, E> : detail::Result<void, E> {

	using detail::Result<void, E>::Result;

	template<typename F>
	Result<typename std::result_of<F()>::type, E> map(const F& f) const {
		if(this->is_ok()) {
			return Ok(f());
		}
		return Err(this->_error.get());
	}

	template<typename F>
	Result<void, typename std::result_of<F(E)>::type> map_err(const F& f) const {
		if(this->is_ok()) {
			return Ok();
		}
		return Err(f(this->_error.get()));
	}

};

template<typename T>
struct Result<T, void> : detail::Result<T, void> {

	using detail::Result<T, void>::Result;

	auto unwrap_or(const T& f) const {
		return this->is_ok() ? this->_value.get() : f;
	}

	template<typename F>
	Result<typename std::result_of<F(T)>::type, void> map(const F& f) const {
		if(this->is_ok()) {
			return Ok(f(this->_value.get()));
		}
		return Err();
	}

	template<typename F>
	Result<T, typename std::result_of<F()>::type> map_err(const F& f) const {
		if(this->is_ok()) {
			return Ok(this->_value.get());
		}
		return Err(f());
	}
};


}

#endif // Y_CORE_RESULT_H
