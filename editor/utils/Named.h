/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef EDITOR_UTILS_NAMED_H
#define EDITOR_UTILS_NAMED_H

#include <editor/editor.h>

#include <y/core/String.h>

namespace editor {

template<typename T>
class Named {
    public:
        Named() = default;

        Named(std::string_view name, T&& obj) : _name(name), _obj(std::move(obj)) {
        }

        explicit Named(T&& obj) : Named("unamed", std::move(obj)) {
        }

        const core::String& name() const {
            return _name;
        }

        const T& obj() const {
            return _obj;
        }

        T& obj() {
            return _obj;
        }

        operator T&() {
            return _obj;
        }

        operator const T&() const {
            return _obj;
        }

        Named& operator=(T&& obj) {
            _obj = std::move(obj);
            return *this;
        }

    private:
        core::String _name;
        T _obj;
};

template<typename T>
Named(std::string_view, T&&) -> Named<T>;

}

#endif // EDITOR_UTILS_NAMED_H

