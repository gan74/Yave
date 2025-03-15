/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_SCRIPT_SCRIPTVM_H
#define YAVE_SCRIPT_SCRIPTVM_H

#include <yave/yave.h>

#include <y/core/String.h>
#include <y/core/Result.h>

struct lua_State;

namespace yave {

enum class ScriptTypeIndex : u32 {
    invalid_index = u32(-1),
};

namespace detail {
ScriptTypeIndex next_script_type_index();
}

template<typename T>
ScriptTypeIndex script_type_index() {
    static_assert(!std::is_const_v<T> && !std::is_reference_v<T>);
    static ScriptTypeIndex type(detail::next_script_type_index());
    return type;
}

class ScriptVM : NonMovable {
    public:
        ScriptVM();
        ~ScriptVM();

        core::Result<void, core::String> run(const core::String& script);

    private:
        lua_State* _state = nullptr;
};

}


#endif // YAVE_SCRIPT_SCRIPTVM_H

