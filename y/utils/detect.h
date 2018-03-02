/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef Y_UTILS_DETECT_H
#define Y_UTILS_DETECT_H

// Should be replaced by std::is_detected, once it gets implemented
namespace y {

namespace detail {
template<typename Default, typename AlwaysVoid, template<typename...> typename Op, typename... Args>
struct detector {
  using value_t = std::false_type;
  using type = Default;
};

template<typename Default, template<typename...> typename Op, typename... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
  // Note that std::void_t is a C++17 feature
  using value_t = std::true_type;
  using type = Op<Args...>;
};

struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const&) = delete;
    void operator=(nonesuch const&) = delete;
};

} // namespace detail

template<template<typename...> typename Op, typename... Args>
using is_detected = typename detail::detector<detail::nonesuch, void, Op, Args...>::value_t;

template<template<typename...> typename Op, typename... Args>
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

}


#endif // Y_UTILS_DETECT_H
