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
#ifndef Y_FORMAT_H
#define Y_FORMAT_H

#include <y/core/Span.h>
#include <y/core/String.h>

#include <y/utils/traits.h>

#include <format>
#include <string_view>
#include <concepts>


#undef y_fatal
#define y_fatal_no_fmt(msg) y::fatal((msg), __FILE__, __LINE__)
#define y_fatal(...) y_fatal_no_fmt(y::fmt_c_str(__VA_ARGS__))



namespace y {
namespace detail {
core::MutableSpan<char> alloc_fmt_buffer();
}

template<typename... Args>
const char* fmt_c_str(std::format_string<Args...> fmt_str, Args&&... args);

template<typename... Args>
std::string_view fmt(std::format_string<Args...> fmt_str, Args&&... args) {
    if constexpr(sizeof...(args) == 0) {
        return fmt_str.get();
    }
    try {
        auto buffer = detail::alloc_fmt_buffer();
        const auto res = std::format_to_n(buffer.begin(), buffer.size() - 1, fmt_str,y_fwd(args)...);

        if(usize(res.size + 1) >= buffer.size()) {
            *(res.out - 1) = *(res.out - 2) = *(res.out - 3) = '.';
        }

        *res.out = 0;
        return std::string_view(buffer.begin(), res.out - buffer.data());
    } catch(std::exception& e) {
        y_fatal("fmt failed: {}", e.what());
    }
}

template<typename... Args>
const char* fmt_c_str(std::format_string<Args...> fmt_str, Args&&... args) {
    return fmt(fmt_str, y_fwd(args)...).data();
}

inline const char* fmt_c_str(const char* str) {
    return str;
}

template<typename... Args>
std::string_view fmt_into(core::String& out, std::format_string<Args...> fmt_str, Args&&... args) {
    try {
        const usize start = out.size();
        std::format_to(std::back_inserter(out), fmt_str, y_fwd(args)...);
        return out.sub_str(start, out.size() - start);
    } catch(std::exception& e) {
        y_fatal("fmt failed: {}", e.what());
    }
}

template<typename... Args>
core::String fmt_to_owned(std::format_string<Args...> fmt_str, Args&&... args) {
    core::String buffer;
    fmt_into(buffer, fmt_str, y_fwd(args)...);
    return buffer;
}




template<typename T>
core::String core::String::operator+(const T& r) const {
    core::String l(*this);
    if constexpr(has_append<core::String, T>) {
        l += y_fwd(r);
    } else {
        fmt_into(l, "{}", r);
    }
    return l;
}


template<typename T>
concept is_formattable = requires(T& v, std::format_context ctx) {
    std::formatter<std::remove_cvref_t<T>>().format(v, ctx);
};

}




// https://www.en.cppreference.com/w/cpp/utility/format/formattable.html
template<typename T, class Context, typename Formatter = typename Context::template formatter_type<std::remove_const_t<T>>>
concept is_formatable = std::semiregular<Formatter> &&
    requires (Formatter& f, const Formatter& cf, T&& t, Context fc, std::basic_format_parse_context<typename Context::char_type> pc) {
    { f.parse(pc) } -> std::same_as<typename decltype(pc)::iterator>;
    { cf.format(t, fc) } -> std::same_as<typename Context::iterator>;
};



template<>
struct std::formatter<y::core::String, char> : std::formatter<std::string_view, char> {
    template<typename Ctx>
    auto format(const y::core::String& str, Ctx& ctx) const {
        return std::formatter<std::string_view, char>::format(str.view(), ctx);
    }
};

// Sketchy
template<typename T, typename C> requires(std::is_enum_v<T>)
struct std::formatter<T, C> : std::formatter<std::underlying_type_t<T>, C> {
    template<typename Ctx>
    auto format(const T& t, Ctx& ctx) const {
        return std::formatter<std::underlying_type_t<T>, C>::format(std::underlying_type_t<T>(t), ctx);
    }
};

template<typename T, typename C> requires(y::is_iterable<T>)
struct std::formatter<T, C> : std::formatter<y::element_type_t<T>, C> {
    template<typename Ctx>
    auto format(const T& t, Ctx& ctx) const {
        std::format_to(ctx.out(), "[");
        bool separator = false;
        for(const auto& e : t) {
            if(separator) {
                std::format_to(ctx.out(), ", ");
            }
            separator = true;
            std::formatter<y::element_type_t<T>, C>::format(e, ctx);
        }
        return std::format_to(ctx.out(), "]");
    }
};


#endif // Y_FORMAT_H

