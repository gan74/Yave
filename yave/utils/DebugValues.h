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
#ifndef YAVE_UTILS_DEBUGVALUES_H
#define YAVE_UTILS_DEBUGVALUES_H

#include <yave/yave.h>

#include <y/core/String.h>
#include <y/core/HashMap.h>

#include <variant>

namespace yave {

class DebugValues : NonMovable {
    public:
        using Value = std::variant<bool, int, float>;

        template<typename T>
        T value(std::string_view name, T default_val = {}) {
            auto it = _values.find(name);
            if(it == _values.end()) {
                using pair_type = typename decltype(_values)::value_type;
                it = _values.insert(pair_type(name, Value(default_val))).first;
            }
            return std::visit([](const auto& inner) { return T(inner); }, it->second);
        }

        core::FlatHashMap<core::String, Value>& all_values() {
            return _values;
        }

    private:
        core::FlatHashMap<core::String, Value> _values;

};

}


#endif // YAVE_UTILS_DEBUGVALUES_H

