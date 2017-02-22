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

#include "LuaComponent.h"


namespace yave {

static void err(const sol::error& e) {
	log_msg("Lua error: "_s + e.what(), LogType::Error);
}

static void check(const sol::protected_function_result& res) {
	if(!res.valid()) {
		err(res);
	}
}

template<typename T, typename... Args>
static void run(T&& p, Args&&... args) {
	if(p.valid()) {
		check(
			sol::protected_function(std::forward<T>(p))
			(std::forward<Args>(args)...));
	}
}



LuaComponent::LuaComponent(sol::state& state, const char* script) {
	if(script) {
		auto res = state.do_string(script);
		_component = res.valid() ? res : (err(res), _component = state.create_table());
	}
}


void LuaComponent::update(const core::Duration& d) {
	update(d.to_secs());
}

void LuaComponent::update(float dt) {
	run(_component["update"], _component, dt);
}


}
