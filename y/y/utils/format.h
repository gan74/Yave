/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;

            using value_type = char;
            using reference = char&;
            using pointer = char*;

            inline Iterator& operator++() {
                ++index;
                return *this;
            }

            inline Iterator operator++(int) {
                auto it = *this;
                ++index;
                return it;
            }

            inline char& operator*() const {
                return index < buffer_size ? buffer[index] : overflow;
            }

            char* buffer = nullptr;
            usize buffer_size = 0;
            usize index = 0;
            mutable char overflow = 0;
        } it {
            buffer.data(), buffer.size() - 1,
            0, 0
        };

        static_assert(std::output_iterator<Iterator, const char&>);

        it = std::format_to(it, fmt_str,y_fwd(args)...);

        if(it.index > it.buffer_size) {
            Y_TODO(find something better)
            y_breakpoint;
            it.index = it.buffer_size;
            it.buffer[it.index - 1] = it.buffer[it.index - 2] = it.buffer[it.index - 3] = '.';
        }

        it.buffer[it.index] = 0;
        return std::string_view(it.buffer, it.index);
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
inline core::String operator+(core::String l, T&& r) {
    if constexpr(has_append_v<core::String, T>) {
        l += y_fwd(r);
    } else {
        fmt_into(l, "{}", r);
    }
    return l;
}

inline core::String operator+(std::string_view l, const core::String& r) {
    core::String s;
    s.set_min_capacity(l.size() + r.size());
    s += l;
    s += r;
    return s;
}

}



template<typename T, typename C>
concept Formatable = requires(T t) {
    { std::formatter<T, C>{}.parse(std::declval<std::format_parse_context&>()) };
};

template<typename T, typename C> requires(y::is_iterable_v<T> && !Formatable<T, C>)
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


#endif // Y_FORMAT_H

