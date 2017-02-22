/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "lua.h"

#include <y/math/Matrix.h>
#include <y/core/String.h>

#include <yave/yave.h>

namespace yave {
namespace lua {

template<typename T>
static void build_empty(T& t) {
	new(&t) T();
}

template<typename T>
static void build_from_table(T& v, const sol::table& t) {
	build_empty(v);

	usize i = 0;
	for(auto& e : v) {
		e = t[++i];
	}
}


template<typename T>
static std::string to_string(T& v) {
	auto e = v.end();
	--e;
	core::String str = "(";
	for(auto it = v.begin(); it != e; ++it) {
		str = str + *it + ", ";
	}
	return std::string(str + *e + ")");
}




template<typename T, usize I = T::size()>
struct VecComponents : VecComponents<T, I - 1> {
	static constexpr char components[] = {'x', 'y', 'z', 'w'};

	template<typename... Args>
	VecComponents(Args&&... args) :
			VecComponents<T, I - 1>(
				std::forward<Args>(args)...,
				std::string(1, components[I - 1]), sol::property(T::template get<I - 1>, T::template set<I - 1>)){
	}
};

template<typename T>
struct VecComponents<T, 0> {
	template<typename... Args>
	VecComponents(Args&&... args) : type(std::forward<Args>(args)...) {
	}

	sol::usertype<T> type;
};


template<typename T>
static void load_vec(sol::state& state, const std::string& name) {
	sol::usertype<T> type = VecComponents<T>(
			sol::initializers(build_empty<T>, build_from_table<T>),

			"length2", &T::length2,
			"length", &T::length,
			"dot", &T::dot,
			"cross", &T::cross,
			"normalize", &T::normalize,
			"normalized", &T::normalized,

			"__add",  [](const T& a, const T& b) { return a + b; },
			"__sub",  [](const T& a, const T& b) { return a - b; },
			"__mul",  [](const T& a, const T& b) { return a * b; },
			"__div",  [](const T& a, const T& b) { return a / b; },

			"__tostring", to_string<T>
		).type;

	state.set_usertype(name, type);
}

void load_user_types(sol::state& state) {
	using namespace y::math;

	load_vec<Vec4>(state, "Vec4");
	load_vec<Vec3>(state, "Vec3");
	load_vec<Vec2>(state, "Vec2");
}


sol::state create_state() {
	sol::state state;
	state.open_libraries();

	load_user_types(state);

	return state;
}

}
}
