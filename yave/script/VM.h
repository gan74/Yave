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
#ifndef YAVE_SCRIPT_VM_H
#define YAVE_SCRIPT_VM_H

#include <yave/yave.h>

#include <y/reflect/reflect.h>

#include <y/core/String.h>
#include <y/core/Result.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace yave {
namespace script {

namespace detail {
template<typename T, typename M>
auto property(M T::* member) {
    if constexpr(std::is_same_v<M, core::String>) {
        return sol::property(
            [=](T* obj, std::string_view view) { (obj->*member) = view; },
            [=](T* obj) { return (obj->*member).view(); }
        );
    } else {
        return member;
    }
}
}

class VM : NonCopyable {
    public:
        struct Error {
            core::String msg;
        };

        VM();

        core::Result<void, Error> run(const char* code);


        template<typename T>
        void set_global(const char* name, const T& value) {
            if constexpr(std::is_same_v<T, core::String>) {
                _state[name] = value.view();
            } else {
                _state[name] = value;
            }
        }

        template<typename T>
        void bind_type() {
            sol::usertype<T> type = _state.new_usertype<T>(T::_y_reflect_type_name);
            reflect::explore_members<T>([&](std::string_view name, auto member) {
                type[name] = detail::property(member);
            });
        }

    private:
        sol::state _state;
};

}
}

#endif // YAVE_SCRIPT_VM_H
