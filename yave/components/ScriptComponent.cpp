/*******************************
Copyright (c) 2016-2022 Gr�goire Angerand

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

#include "ScriptComponent.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {


void ScriptComponent::set_code(core::String code) {
    _code = std::move(code);
    _status = Status::Uninitialized;
    _state = sol::state();
    _state.open_libraries();
}

bool ScriptComponent::start() {
    if(_status == Status::Uninitialized) {
        if(auto result = _state.safe_script(_code); !result.valid()) {
            log_msg(fmt("Lua error: %", std::string_view(sol::to_string(result.status()))), Log::Error);
            _status = Status::Error;
            return false;
        }
        _status = Status::Started;
    }
    return _status == Status::Started;
}

sol::state& ScriptComponent::state() {
    y_debug_assert(_status == Status::Started);
    return _state;
}

}

