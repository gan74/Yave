/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef Y_SERDE3_SERDE_H
#define Y_SERDE3_SERDE_H

#include <y/utils.h>
#include <y/utils/recmacros.h>

#include <tuple>
#include <string_view>

namespace y {
namespace serde3 {

class WritableArchive;
class ReadableArchive;

template<typename T>
struct NamedObject {
    T& object;
    const std::string_view name;

    inline constexpr NamedObject(T& t, std::string_view n) : object(t), name(n) {
    }

    inline constexpr NamedObject<const T> make_const_ref() const {
        return NamedObject<const T>(object, name);
    }

    inline constexpr NamedObject<T> make_ref() const {
        return *this;
    }
};

#define y_serde3_create_item(object) y::serde3::NamedObject{object, #object},

#define y_serde3_refl_qual(qual, ...) /*template<typename = void> inline*/ auto _y_serde3_refl() qual { return std::tuple{Y_REC_MACRO(Y_MACRO_MAP(y_serde3_create_item, __VA_ARGS__))}; }


#define y_serde3(...)                               \
    y_serde3_refl_qual(/* */, __VA_ARGS__)          \
    y_serde3_refl_qual(const, __VA_ARGS__)

#define y_no_serde3() static constexpr int _y_serde3_no_serde = 0;

}
}

#endif // Y_SERDE3_SERDE_H

