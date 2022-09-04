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

#include <y/core/Result.h>

#include "lua_helpers.h"

namespace yave {
namespace script {

class VM : NonCopyable {
    public:
        struct Error {
            core::String msg;
        };

        VM() = default;
        ~VM();

        VM(VM&& other);
        VM& operator=(VM&& other);


        static VM create();

        void gc();

        core::Result<void, Error> run(const char* code);



        template<typename T>
        void set_global(const char* name, const T& value) {
            lua::push_value(_l, value);
            lua_setglobal(_l, name);
        }

        template<typename T>
        void bind_type() {
            lua::create_type_metatable<T>(_l, [](lua_State*) { return T{}; });
        }


    public:
        lua_State* state() {
            return _l;
        }

    private:
        void swap(VM& other);

        lua_State* _l = nullptr;
};

}
}

#endif // YAVE_SCRIPT_VM_H
