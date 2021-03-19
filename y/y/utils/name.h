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
#ifndef Y_UTILS_NAME_H
#define Y_UTILS_NAME_H

#include <y/defines.h>

#include <string_view>

namespace y {
namespace detail {
// Trick from https://github.com/Neargye/nameof
template<typename... T>
constexpr std::string_view ct_type_name() {
#if defined(Y_CLANG)
    return std::string_view{__PRETTY_FUNCTION__ + 49, sizeof(__PRETTY_FUNCTION__) - 52};
#elif defined(Y_GCC)
    return std::string_view{__PRETTY_FUNCTION__ + 64, sizeof(__PRETTY_FUNCTION__) - 116};
#elif defined(Y_MSVC)
    return std::string_view{__FUNCSIG__ + 98, sizeof(__FUNCSIG__) - 106};
#else
static_assert(false, "ct_type_name is not supported");
#endif
}
}

template<typename T>
constexpr std::string_view ct_type_name() {
    return detail::ct_type_name<T>();
}

static_assert(ct_type_name<int>() == "int");
static_assert(ct_type_name<float>() == "float");

// This is compiler dependent =(
//static_assert(ct_type_name<std::string_view>() == "std::basic_string_view<char,std::char_traits<char>>");

}


#endif // Y_UTILS_NAME_H

